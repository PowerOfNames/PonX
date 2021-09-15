#include "pxpch.h"
#include "VulkanContext.h"

//#include "VulkanLookups.h"
#include "VulkanDebug.h"
#include "VulkanImage.h"


#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>


namespace Povox {

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		PX_CORE_ASSERT(windowHandle, "Window handle is null");

		glfwSetWindowUserPointer(windowHandle, this);
		glfwSetFramebufferSizeCallback(windowHandle, FramebufferResizeCallback);
	}

	void VulkanContext::Init()
	{
		PX_PROFILE_FUNCTION();


		CreateInstance();
		SetupDebugMessenger();

		CreateSurface();

	// Device picking and creation -> happens automatically
		m_Device = CreateRef<VulkanDevice>();
		m_Device->AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		m_Device->PickPhysicalDevice(m_Instance, m_Surface);
		m_Device->CreateLogicalDevice(m_Surface, m_ValidationLayers);

	// Swapchain creation -> needs window size either automatically or maybe by setting values inside the vulkan renderer
		m_Swapchain = CreateRef<VulkanSwapchain>();
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		m_Swapchain->Create(m_Device->GetLogicalDevice(), m_Surface, m_Device->QuerySwapchainSupport(m_Device->GetPhysicalDevice(), m_Surface), m_Device->FindQueueFamilies(m_Device->GetPhysicalDevice(), m_Surface), width, height);
		m_Swapchain->CreateImagesAndViews(m_Device->GetLogicalDevice());

	// Renderpass creation -> automatically after swapchain creation
		m_RenderPass = CreateRef<VulkanRenderPass>();
		m_RenderPass->Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Swapchain->GetImageFormat());
		// descriptorpools and sets are dependent on the set layout which will be dynamic dependend on the use case of the vulkan renderer (atm its just the ubo and texture sampler)
		m_DescriptorPool = CreateRef<VulkanDescriptorPool>();
		m_DescriptorPool->SetLayout(m_Device->GetLogicalDevice());
	
		// default pipeline automatic, need to further look up when the pipeline changes inside the app during runtime
		m_GraphicsPipeline = CreateRef<VulkanPipeline>();
		m_GraphicsPipeline->Create(m_Device->GetLogicalDevice(), m_Swapchain->GetExtent2D(), m_RenderPass->Get(), m_DescriptorPool->GetLayout());
		//also initial, pool for differend queue families, usage a bit unsure
		CreateCommandPool();
		CreateDepthResources();
		// default initially, but can change, framebuffer main bridge to after effects stuff
		CreateFramebuffers();
		// sampler initial, image and image view creation maybe if its used or so
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();

		m_DescriptorPool->CreatePool(m_Device->GetLogicalDevice(), m_Swapchain->GetImages().size());
		m_DescriptorPool->CreateSets(m_Device->GetLogicalDevice(), m_Swapchain->GetImages().size(), m_UniformBuffers, m_TextureImageView, m_TextureSampler);

		// I create all needed command buffers initially and use them when needed in the main draw call
		CreateCommandBuffers();

		// initial, dont know exactly when to touch those
		CreateSyncObjects();
	}

	void VulkanContext::RecreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_WindowHandle, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_WindowHandle, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Device->GetLogicalDevice());

		CleanupSwapchain();
		m_Swapchain->Create(m_Device->GetLogicalDevice(), m_Surface, m_Device->QuerySwapchainSupport(m_Device->GetPhysicalDevice(), m_Surface), m_Device->FindQueueFamilies(m_Device->GetPhysicalDevice(), m_Surface), width, height);
		m_Swapchain->CreateImagesAndViews(m_Device->GetLogicalDevice());
		m_RenderPass->Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_Swapchain->GetImageFormat());

		m_GraphicsPipeline->Create(m_Device->GetLogicalDevice(), m_Swapchain->GetExtent2D(), m_RenderPass->Get(), m_DescriptorPool->GetLayout());	// avoid recreating here by using dynamic state for viewport and scissors
		CreateDepthResources();
		CreateFramebuffers();
		CreateUniformBuffers();
		m_DescriptorPool->CreatePool(m_Device->GetLogicalDevice(), m_Swapchain->GetImages().size());
		m_DescriptorPool->CreateSets(m_Device->GetLogicalDevice(), m_Swapchain->GetImages().size(), m_UniformBuffers, m_TextureImageView, m_TextureSampler);
		CreateCommandBuffers();
	}

	void VulkanContext::CleanupSwapchain()
	{
		vkDeviceWaitIdle(m_Device->GetLogicalDevice());


		vkDestroyImageView(m_Device->GetLogicalDevice(), m_DepthImageView, nullptr);
		vkDestroyImage(m_Device->GetLogicalDevice(), m_DepthImage, nullptr);
		vkFreeMemory(m_Device->GetLogicalDevice(), m_DepthImageMemory, nullptr);

		for (auto framebuffer : m_SwapchainFramebuffers)
		{
			vkDestroyFramebuffer(m_Device->GetLogicalDevice(), framebuffer, nullptr);
		}
		vkFreeCommandBuffers(m_Device->GetLogicalDevice(), m_CommandPoolGraphics, static_cast<uint32_t>(m_CommandBuffersGraphics.size()), m_CommandBuffersGraphics.data());
		m_GraphicsPipeline->Destroy(m_Device->GetLogicalDevice());
		m_GraphicsPipeline->DestroyLayout(m_Device->GetLogicalDevice());
		m_RenderPass->Destroy(m_Device->GetLogicalDevice());

		for (auto imageView : m_Swapchain->GetImageViews())
		{
			vkDestroyImageView(m_Device->GetLogicalDevice(), imageView, nullptr);
		}		
		m_Swapchain->Destroy(m_Device->GetLogicalDevice());

		for (size_t i = 0; i < m_Swapchain->GetImages().size(); i++)
		{
			vkDestroyBuffer(m_Device->GetLogicalDevice(), m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_Device->GetLogicalDevice(), m_UniformBuffersMemory[i], nullptr);
		}

		m_DescriptorPool->DestroyPool(m_Device->GetLogicalDevice());
	}

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		
		CleanupSwapchain();

		vkDestroySampler(m_Device->GetLogicalDevice(), m_TextureSampler, nullptr);
		vkDestroyImageView(m_Device->GetLogicalDevice(), m_TextureImageView, nullptr);
		vkDestroyImage(m_Device->GetLogicalDevice(), m_TextureImage, nullptr);
		vkFreeMemory(m_Device->GetLogicalDevice(), m_TextureImageMemory, nullptr);

		m_DescriptorPool->DestroyLayout(m_Device->GetLogicalDevice());

		m_IndexBuffer->Destroy(m_Device->GetLogicalDevice());
		m_VertexBuffer->Destroy(m_Device->GetLogicalDevice());

		vkFreeMemory(m_Device->GetLogicalDevice(), m_IndexBuffer->GetMemory(), nullptr);
		vkFreeMemory(m_Device->GetLogicalDevice(), m_VertexBuffer->GetMemory(), nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device->GetLogicalDevice(), m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device->GetLogicalDevice(), m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_Device->GetLogicalDevice(), m_InFlightFence[i], nullptr);
		}
		vkDestroyCommandPool(m_Device->GetLogicalDevice(), m_CommandPoolTransfer, nullptr);
		vkDestroyCommandPool(m_Device->GetLogicalDevice(), m_CommandPoolGraphics, nullptr);

		m_Device->Destroy();

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		//glfw termination and window deletion at last
	}

	void VulkanContext::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();


		vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFence[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_Swapchain->Get(), UINT64_MAX /*disables timeout*/, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();
			return;
		}
		else
		{
			PX_CORE_ASSERT(!((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)), "Failed to acquire swap chain image!");
		}

		// check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (m_ImagesInFlight[m_CurrentFrame] != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFence[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		}
		else // mark the image as now being used now by this frame
		{
			m_ImagesInFlight[m_CurrentFrame] = m_InFlightFence[m_CurrentFrame];
		}

		UpdateUniformBuffer(imageIndex);			// Update function

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffersGraphics[imageIndex];		// command buffer usage

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFence[m_CurrentFrame]);

		PX_CORE_VK_ASSERT(vkQueueSubmit(m_Device->GetQueueFamilies().GraphicsQueue, 1, &submitInfo, m_InFlightFence[m_CurrentFrame]), VK_SUCCESS, "Failed to submit draw render buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapchains[] = { m_Swapchain->Get() };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;		// optional

		result = vkQueuePresentKHR(m_Device->GetQueueFamilies().PresentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
		{
			m_FramebufferResized = false;
			RecreateSwapchain();
		}
		else
		{
			PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");
		}

		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}


// Instance
	void VulkanContext::CreateInstance()
	{
		if (PX_ENABLE_VK_VALIDATION_LAYERS && !CheckValidationLayerSupport())
		{
			PX_CORE_WARN("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName	= "Povox Vulkan Renderer";
		appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName			= "Povox";
		appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (PX_ENABLE_VK_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount	= static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames	= m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext				= static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
		}
		else
		{
			createInfo.enabledLayerCount	= 0;

			createInfo.pNext				= nullptr;
		}

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount	= extensions.size();
		createInfo.ppEnabledExtensionNames	= extensions.data();

		PX_CORE_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance), VK_SUCCESS, "Failed to create Vulkan Instance");
	}


// Surface
	void VulkanContext::CreateSurface()
	{
		PX_CORE_VK_ASSERT(glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface), VK_SUCCESS, "Failed to create window surface!");
	}


// Framebuffers
	// move to VulkanFramebuffer
	void VulkanContext::CreateFramebuffers()
	{
		m_SwapchainFramebuffers.resize(m_Swapchain->GetImageViews().size());
		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); i++)
		{
			std::array<VkImageView, 2> attachments = { m_Swapchain->GetImageViews()[i], m_DepthImageView };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass		= m_RenderPass->Get();			// needs to be compatible (roughly, use the same number and type of attachments)
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());					// The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
			framebufferInfo.pAttachments	= attachments.data();
			framebufferInfo.width			= m_Swapchain->GetExtent2D().width;
			framebufferInfo.height			= m_Swapchain->GetExtent2D().height;
			framebufferInfo.layers			= 1;					// number of layers in image array

			PX_CORE_VK_ASSERT(vkCreateFramebuffer(m_Device->GetLogicalDevice(), &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]), VK_SUCCESS, "Failed to create framebuffer!");
		}
	}

	void VulkanContext::CreateDepthResources()
	{
		VkFormat depthFormat = VulkanUtils::FindDepthFormat(m_Device->GetPhysicalDevice());
		VulkanImage::Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_DepthImage, m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImageMemory);
		VulkanImageView::Create(m_Device->GetLogicalDevice(), m_DepthImageView, m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		TransitionImageLayout(m_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);												// ---------- Command
	}

	bool VulkanContext::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

// Command pools
	// move to VulkanCommandBuffer, but needs more research on what this exactly does and how to use
	void VulkanContext::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilies = m_Device->FindQueueFamilies(m_Device->GetPhysicalDevice(), m_Surface);

		VkCommandPoolCreateInfo commandPoolGraphicsInfo{};
		commandPoolGraphicsInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolGraphicsInfo.queueFamilyIndex	= queueFamilies.GraphicsFamily.value();
		commandPoolGraphicsInfo.flags				= 0;		// optional

		VkCommandPoolCreateInfo commandPoolTransferInfo{};
		commandPoolTransferInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolTransferInfo.queueFamilyIndex	= queueFamilies.TransferFamily.value();
		commandPoolTransferInfo.flags				= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;


		PX_CORE_VK_ASSERT(vkCreateCommandPool(m_Device->GetLogicalDevice(), &commandPoolGraphicsInfo, nullptr, &m_CommandPoolGraphics), VK_SUCCESS, "Failed to create graphics command pool!");

		PX_CORE_VK_ASSERT(vkCreateCommandPool(m_Device->GetLogicalDevice(), &commandPoolTransferInfo, nullptr, &m_CommandPoolTransfer), VK_SUCCESS, "Failed to create transfer command pool!");
	}


	void VulkanContext::CreateTextureImage()
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load("assets/textures/logo.png", &width, &height, &channels, STBI_rgb_alpha);
		PX_CORE_ASSERT(pixels, "Failed to load texture!");
		VkDeviceSize imageSize = width * height * 4;

		PX_CORE_TRACE("Image width : '{0}', height '{1}'", width, height);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VulkanBuffer::CreateBuffer(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device->GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_Device->GetLogicalDevice(), stagingBufferMemory);

		stbi_image_free(pixels);

		VulkanImage::Create(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), m_TextureImage, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImageMemory);

		TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));																		//-------- Command
		TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_Device->GetLogicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_Device->GetLogicalDevice(), stagingBufferMemory, nullptr);
	}

	void VulkanContext::CreateTextureImageView()
	{
		 VulkanImageView::Create(m_Device->GetLogicalDevice(), m_TextureImageView, m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void VulkanContext::CreateTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter				= VK_FILTER_LINEAR;
		samplerInfo.minFilter				= VK_FILTER_LINEAR;
		samplerInfo.addressModeU			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW			= VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable		= VK_TRUE;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_Device->GetPhysicalDevice(), &properties);
		samplerInfo.maxAnisotropy			= properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor				= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable			= VK_TRUE ;
		samplerInfo.compareOp				= VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias				= 0.0f;
		samplerInfo.maxLod					= 0.0f;
		samplerInfo.minLod					= 0.0f;

		PX_CORE_VK_ASSERT(vkCreateSampler(m_Device->GetLogicalDevice(), &samplerInfo, nullptr, &m_TextureSampler), VK_SUCCESS, "Failed to create texture sampler!");
	}

// Command
	void VulkanContext::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_CommandPoolGraphics);

		VkImageMemoryBarrier barrier{};
		barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image							= image;
		barrier.oldLayout						= oldLayout;
		barrier.newLayout						= newLayout;
		barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (HasStencilComponent(format))
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel	= 0;
		barrier.subresourceRange.levelCount		= 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount		= 1;

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
		
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			PX_CORE_ASSERT(false, "Unsupported layout transition!");
		}
			
		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndSingleTimeCommands(commandBuffer, m_Device->GetQueueFamilies().GraphicsQueue, m_CommandPoolGraphics);
	}

// Command
	void VulkanContext::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_CommandPoolTransfer);

		VkBufferImageCopy region{};
		region.bufferOffset						= 0;
		region.bufferImageHeight				= 0;
		region.bufferRowLength					= 0;
		region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel		= 0;
		region.imageSubresource.baseArrayLayer	= 0;
		region.imageSubresource.layerCount		= 1;
		region.imageOffset						= { 0, 0, 0 };
		region.imageExtent						= { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(commandBuffer, m_Device->GetQueueFamilies().TransferQueue, m_CommandPoolTransfer);
	}

	void VulkanContext::CreateUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(m_Swapchain->GetImages().size());
		m_UniformBuffersMemory.resize(m_Swapchain->GetImages().size());

		for (size_t i = 0; i < m_Swapchain->GetImages().size(); i++)
		{
			VulkanBuffer::CreateBuffer(m_Device->GetLogicalDevice(), m_Device->GetPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);
		}
	}

// Command
	void VulkanContext::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_CommandPoolTransfer);

		VkBufferCopy copyRegion{};
		copyRegion.size	= size;

		vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer, m_Device->GetQueueFamilies().TransferQueue, m_CommandPoolTransfer);
	}

	// move to VulkanCommandBuffer

	VkCommandBuffer VulkanContext::BeginSingleTimeCommands(VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo bufferAllocInfo{};
		bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocInfo.commandPool = commandPool;
		bufferAllocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &bufferAllocInfo, &commandBuffer);

		VkCommandBufferBeginInfo bufferBeginInfo{};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo);

		return commandBuffer;
	}

	void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool commandPool)
	{
		vkEndCommandBuffer(commandBuffer);


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;


		// with fences I could submit multiple transfers and wait for all of them -> enable for more possible optimization opportunities
		PX_CORE_VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE), VK_SUCCESS, "Failed to submit copy buffer!");
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(m_Device->GetLogicalDevice(), commandPool, 1, &commandBuffer);
	}

	void VulkanContext::CreateCommandBuffers()
	{
		m_CommandBuffersGraphics.resize(m_SwapchainFramebuffers.size());


		VkCommandBufferAllocateInfo allocInfoGraphics{};
		allocInfoGraphics.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfoGraphics.commandPool			= m_CommandPoolGraphics;
		allocInfoGraphics.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		allocInfoGraphics.commandBufferCount	= (uint32_t)m_CommandBuffersGraphics.size();

		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &allocInfoGraphics, m_CommandBuffersGraphics.data()), VK_SUCCESS, "Failed to create command buffers!");

		for (size_t i = 0; i < m_CommandBuffersGraphics.size(); i++)
		{
		// start begin-block
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags				= 0;		// optional
			beginInfo.pInheritanceInfo	= nullptr;	// optional, only used if buffer is secondary

			PX_CORE_VK_ASSERT(vkBeginCommandBuffer(m_CommandBuffersGraphics[i], &beginInfo), VK_SUCCESS, "Failed to begin recording command buffer!");						// Begin command buffer

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass->Get();																				// render pass
			renderPassInfo.framebuffer = m_SwapchainFramebuffers[i];																		// framebuffers
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_Swapchain->GetExtent2D();																	// extent
			std::array<VkClearValue, 2> clearColor = {};
			clearColor[0].color = { 0.25f, 0.25f, 0.25f, 1.0f };																			// clearcolor (framebuffer)
			clearColor[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColor.size());
			renderPassInfo.pClearValues = clearColor.data();


			// All command need the respective command buffer (graphics here)

			vkCmdBeginRenderPass(m_CommandBuffersGraphics[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);																	// Begin Render Pass
		// end begin-block
			//Vertex stuff
			VkBuffer vertexBuffers[] = { m_VertexBuffer->Get() };																			// VertexBuffer needed
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindPipeline(m_CommandBuffersGraphics[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->Get());																	// Bind command for pipeline		|needs (bind point (graphisc here)), pipeline
			vkCmdBindVertexBuffers(m_CommandBuffersGraphics[i], 0, 1, vertexBuffers, offsets);																							// Bind command for vertex buffer	|needs (first binding, binding count), buffer array and offset
			vkCmdBindIndexBuffer(m_CommandBuffersGraphics[i], m_IndexBuffer->Get(), 0, VK_INDEX_TYPE_UINT16);																			// Bind command for index buffer	|needs indexbuffer, offset, type		
			vkCmdBindDescriptorSets(m_CommandBuffersGraphics[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->GetLayout(), 0, 1, &m_DescriptorPool->GetSets()[i], 0, nullptr);	// Bind command for descriptor-sets |needs (bindpoint), pipeline-layout, firstSet, setCount, setArray, dynOffsetCount, dynOffsets 

			vkCmdDrawIndexed(m_CommandBuffersGraphics[i], static_cast<uint32_t>(m_IndexBuffer->GetIndices().size()), 1, 0, 0, 0);																			// Draw indexed command				|needs index count, instance count, firstIndex, vertex offset, first instance 

			vkCmdEndRenderPass(m_CommandBuffersGraphics[i]);																												// End Render Pass

			PX_CORE_VK_ASSERT(vkEndCommandBuffer(m_CommandBuffersGraphics[i]), VK_SUCCESS, "Failed to record graphics command buffer!");									// End command buffer
		}
	}

// Semaphores
	// move to VulkanUtils or VulkanSyncObjects
	void VulkanContext::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFence.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImagesInFlight.resize(m_Swapchain->GetImages().size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo createSemaphoreInfo{};
		createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			bool result = false;
			if ((vkCreateSemaphore(m_Device->GetLogicalDevice(), &createSemaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS)
				&& (vkCreateSemaphore(m_Device->GetLogicalDevice(), &createSemaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS)
				&& (vkCreateFence(m_Device->GetLogicalDevice(), &createFenceInfo, nullptr, &m_InFlightFence[i]) == VK_SUCCESS))
				result = true;
			PX_CORE_ASSERT(result, "Failed to create synchronization objects for a frame!");
		}
	}

	//TODO: move outside and create an event?
//Framebuffer resize callback
	void VulkanContext::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
		app->m_FramebufferResized = true;
	}

	bool VulkanContext::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* vLayer : m_ValidationLayers)
		{
			//PX_CORE_INFO("Validation layer: '{0}'", vLayer);
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				//PX_CORE_INFO("available layers: '{0}'", layerProperties.layerName);

				if (strcmp(vLayer, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound) // return false at the first not found layer
				return false;
		}
		return true;
	}

	std::vector<const char*> VulkanContext::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		CheckRequiredExtensions(glfwExtensions, glfwExtensionCount);

		return extensions;
	}

	// CheckRequiredExtensions
	void VulkanContext::CheckRequiredExtensions(const char** glfwExtensions, uint32_t glfWExtensionsCount)
	{
		//PX_CORE_INFO("Required Extensioncount: {0}", glfWExtensionsCount);

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		for (uint32_t i = 0; i < glfWExtensionsCount; i++)
		{
			bool extensionFound = false;
			//PX_CORE_INFO("Required Extension #{0}: {1}", i, glfwExtensions[i]);
			uint32_t index = 1;
			for (const auto& extension : availableExtensions)
			{
				if (strcmp(glfwExtensions[i], extension.extensionName))
				{
					extensionFound = true;
					//PX_CORE_INFO("Extension requested and included {0}", glfwExtensions[i]);
					break;
				}
			}
			if (!extensionFound)
				PX_CORE_WARN("Extension requested but not included: {0}", glfwExtensions[i]);
		}
	}


	void VulkanContext::UpdateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.ProjectionMatrix = glm::perspective(glm::radians(45.0f), m_Swapchain->GetExtent2D().width / (float)m_Swapchain->GetExtent2D().height, 0.1f, 10.0f);
		ubo.ProjectionMatrix[1][1] *= -1; //flips Y

		void* data;
		vkMapMemory(m_Device->GetLogicalDevice(), m_UniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_Device->GetLogicalDevice(), m_UniformBuffersMemory[currentImage]);

		// pushconstants are more efficient for small packages of data;
	}

// Debug
	void VulkanContext::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
	}
	void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}
}