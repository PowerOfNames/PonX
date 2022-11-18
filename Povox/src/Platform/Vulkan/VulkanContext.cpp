#include "pxpch.h"
#include "VulkanContext.h"

#include "VulkanDebug.h"
#include "VulkanDevice.h"


#include "Povox/Core/Application.h"
#include "Povox/Renderer/Renderer.h"



#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	Ref<VulkanDevice> VulkanContext::s_Device = nullptr;
	VkInstance VulkanContext::s_Instance = nullptr;
	VkDescriptorPool VulkanContext::s_DescriptorPool = nullptr;
	VmaAllocator VulkanContext::s_Allocator = nullptr;

	VulkanContext::VulkanContext()
	{
		PX_CORE_TRACE("VulkanContext created!");
	}

	void VulkanContext::Init()
	{
		PX_PROFILE_FUNCTION();


		CreateInstance();
#ifdef PX_ENABLE_VK_VALIDATION_LAYERS
			SetupDebugMessenger();
#endif
		// Device picking and creation -> happens automatically
		s_Device = CreateRef<VulkanDevice>();
		s_Device->PickPhysicalDevice(m_DeviceExtensions);
		m_PhysicalDeviceProperties = s_Device->GetPhysicalDeviceProperties();
		s_Device->CreateLogicalDevice(m_DeviceExtensions, m_ValidationLayers);
		PX_CORE_WARN("Finished Core!");

		// descriptorpools and sets are dependent on the set layout which will be dynamic dependent on the use case of the vulkan renderer (atm its just the ubo and texture sampler)
		CreateMemAllocator();
		PX_CORE_WARN("Finished MemAllocator!");

		s_ResourceFreeQueue.resize(Application::Get()->GetSpecification().MaxFramesInFlight);

		// Renderpass creation -> automatically after swapchain creation
		InitOffscreenRenderPass();
		PX_CORE_WARN("Finished Renderpass!");
		CreateViewportImagesAndViews();

		PX_CORE_TRACE("VulkanContext initialization finished!");
	}

	void VulkanContext::CleanupSwapchain()
	{
		vkDeviceWaitIdle(s_Device->GetVulkanDevice());
		PX_CORE_TRACE("Cleanup swapchain begin!");
		
		//vkDestroyImageView(s_Device->GetVulkanDevice(), m_ViewportImageView, nullptr);
		//vmaDestroyImage(s_Allocator, m_ViewportImage.Image, m_ViewportImage.Allocation);
	}

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		for (auto& queue : s_ResourceFreeQueue)
		{
			for (auto& func : queue)
				func();
		}
		s_ResourceFreeQueue.clear();

		CleanupSwapchain();



		for (size_t i = 0; i < Application::Get()->GetSpecification().MaxFramesInFlight; i++)
		{
			//vmaDestroyBuffer(s_Allocator, m_Frames[i].ObjectBuffer.Buffer, m_Frames[i].ObjectBuffer.Allocation);
		}
		
		vkDestroyDescriptorPool(s_Device->GetVulkanDevice(), m_DescriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(s_Device->GetVulkanDevice(), m_GlobalDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(s_Device->GetVulkanDevice(), m_ObjectDescriptorSetLayout, nullptr);
		
		

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(s_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(s_Instance, nullptr);
	}

	//by Cherno
	void VulkanContext::SubmitResourceFree(std::function<void()>&& func)
	{
		s_ResourceFreeQueue[Renderer::GetCurrentFrameIndex()].push_back(func);
	}

	// this is only used in the next function!
	void VulkanContext::CreateViewportImagesAndViews()
	{
		/*VkExtent3D extent;
		extent.width = static_cast<uint32_t>(m_Swapchain->GetExtent2D().width);
		extent.height = static_cast<uint32_t>(m_Swapchain->GetExtent2D().height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(m_Swapchain->GetImageFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);
		m_ViewportImage = VulkanImageDepr::Create(imageInfo, VMA_MEMORY_USAGE_CPU_ONLY);

		VkImageViewCreateInfo ivInfo = VulkanInits::CreateImageViewInfo(m_Swapchain->GetImageFormat(), m_ViewportImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(s_Device->GetVulkanDevice(), &ivInfo, nullptr, &m_ViewportImageView);*/
	}

	//maybe refactor this in a ways, that I can call a function in the layer, which results to me the image as Image2D or Texture2D, so I can use it. Need to make sure
	// I reuse the resource, and not create a new one every time I call this
	void VulkanContext::CopyOffscreenToViewportImage(VkImage& srcImage)
	{
		//Transitioning
		/*VulkanCommands::ImmidiateSubmitGfx(m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_ViewportImage.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		VulkanCommands::ImmidiateSubmitGfx(m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = srcImage;
				barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		//Image copying
		VulkanCommands::ImmidiateSubmitTrsf(m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageCopy imageCopyRegion{};
				imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.srcSubresource.layerCount = 1;
				imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageCopyRegion.dstSubresource.layerCount = 1;
				imageCopyRegion.extent.width = m_Swapchain->GetExtent2D().width;
				imageCopyRegion.extent.height = m_Swapchain->GetExtent2D().height;
				imageCopyRegion.extent.depth = 1;

				vkCmdCopyImage(cmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_ViewportImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
			});
		//Transitioning
		VulkanCommands::ImmidiateSubmitGfx(m_UploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_ViewportImage.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});*/
	}

	//Core stuff -> those will get called upon init, and stored as statics, so I can call the statically from where I need them (mostly vulkan specific files)
	void VulkanContext::CreateInstance()
	{
		if (PX_ENABLE_VK_VALIDATION_LAYERS && !CheckValidationLayerSupport())
		{
			PX_CORE_WARN("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Povox Vulkan Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Povox";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (PX_ENABLE_VK_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		PX_CORE_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &s_Instance), VK_SUCCESS, "Failed to create Vulkan Instance");
	}			
	void VulkanContext::CreateMemAllocator()
	{
		VmaAllocatorCreateInfo vmaAllocatorInfo = {};
		vmaAllocatorInfo.physicalDevice = s_Device->GetPhysicalDevice();
		vmaAllocatorInfo.device = s_Device->GetVulkanDevice();
		vmaAllocatorInfo.instance = s_Instance;
		vmaCreateAllocator(&vmaAllocatorInfo, &s_Allocator);
	}

	//layer managed
	void VulkanContext::InitOffscreenRenderPass()
	{
		/*VulkanRenderPassBuilder builder = VulkanRenderPassBuilder::Begin();

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_Swapchain->GetImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		builder.AddAttachment(colorAttachment);


		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VulkanUtils::FindDepthFormat(s_Device->GetPhysicalDevice());
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		builder.AddAttachment(depthAttachment);

		
		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::vector<VkAttachmentReference> colorRefs{ colorRef };
		builder.CreateAndAddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colorRefs, depthRef);

		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		builder.AddDependency(dependency);*/	 
	}
	void VulkanContext::UploadMeshes()
	{
		//VulkanVertexBuffer::Create(m_UploadContext, m_DoubleTriangleMesh);
		//VulkanIndexBuffer::Create(m_UploadContext, m_DoubleTriangleMesh);
	}
	// same as shaders, get created an stored inside the layers, and then linked via a game object or material?
	void VulkanContext::LoadTextures()
	{
		/*Texture logo;
		logo.Image2D = VulkanImageDepr::LoadFromFile(m_UploadContext, "assets/textures/newLogo.png", VK_FORMAT_R8G8B8A8_SRGB);

		VkImageViewCreateInfo imageInfo = VulkanInits::CreateImageViewInfo(VK_FORMAT_R8G8B8A8_SRGB, logo.Image2D.Image, VK_IMAGE_ASPECT_COLOR_BIT);
		PX_CORE_VK_ASSERT(vkCreateImageView(s_Device->GetVulkanDevice(), &imageInfo, nullptr, &logo.ImageView), VK_SUCCESS, "Failed to create image view!");

		m_Textures["Logo"] = logo;*/
	}
	


	// might be completely dependent on shaders. Upon startup, I run through the shader lib, and compile every shader, which should also reflect on them and then create 
	// descriptors -> ideally chache them and check if descriptors are the same and can be reused or not
	void VulkanContext::InitDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = 10;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		PX_CORE_VK_ASSERT(vkCreateDescriptorPool(s_Device->GetVulkanDevice(), &poolInfo, nullptr, &s_DescriptorPool), VK_SUCCESS, "Failed to create descriptor pool!");
		PX_CORE_WARN("Created Descriptor pool!");
	}

	//still need to figure out a way, when to determine the count of framebuffers -> dependent on the swapchain images, frames in flight or singe FBs?
	void VulkanContext::CreateOffscreenFramebuffers()
	{
		/*FramebufferSpecification specs;
		specs.Attachements = { {FramebufferTextureFormat::RGBA8, {FramebufferTextureUsage::COLOR, FramebufferTextureUsage::TRANSFER_SRC} }, {FramebufferTextureFormat::Depth, {FramebufferTextureUsage::DEPTH_STENCIL}} };
		specs.Width = m_Swapchain->GetExtent2D().width;
		specs.Height = m_Swapchain->GetExtent2D().height;
		specs.RenderPass = "OffscreenPass";
		m_OffscreenFramebuffers.resize(m_Swapchain->GetImageViews().size());
		for (size_t i = 0; i < m_Swapchain->GetImageViews().size(); i++)
		{			
			VulkanFramebuffer framebuffer(specs);
			FramebufferAttachmentCreateInfo info{ m_Swapchain->GetExtent2D().width, m_Swapchain->GetExtent2D().height, m_Swapchain->GetImageFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT };
			framebuffer.AddAttachment(info);

			info.Format = VulkanUtils::FindDepthFormat(s_Device->GetPhysicalDevice());
			info.ImageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			framebuffer.AddAttachment(info);

			framebuffer.Create();
			m_OffscreenFramebuffers[i] = std::move(framebuffer);
		}*/
	}


	// Extensions and layers
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
	// Debug
	void VulkanContext::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(s_Instance, &createInfo, nullptr, &m_DebugMessenger);
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
