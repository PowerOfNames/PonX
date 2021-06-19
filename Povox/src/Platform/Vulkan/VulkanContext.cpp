#include "pxpch.h"
#include "VulkanContext.h"

#include "VulkanLookups.h"

#include <cstdint>
#include <fstream>

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
		PickPhysicalDevice();
		CreateLogicalDevice();

		CreateSwapchain();
		CreateImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();

		CreateFramebuffers();

		CreateCommandPool();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();

		CreateDescriptorPool();
		CreateDescriptorSets();

		CreateCommandBuffers();

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

		vkDeviceWaitIdle(m_Device);

		CleanupSwapchain();
		CreateSwapchain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline(); // avoid recreating here by using dynamic state for viewport and scissors
		CreateFramebuffers();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();

	}

	void VulkanContext::CleanupSwapchain()
	{
		vkDeviceWaitIdle(m_Device);


		for (auto framebuffer : m_SwapchainFramebuffers)
		{
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
		}
		vkFreeCommandBuffers(m_Device, m_CommandPoolGraphics, static_cast<uint32_t>(m_CommandBuffersGraphics.size()), m_CommandBuffersGraphics.data());
		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);		
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);		
		for (auto imageView : m_SwapchainImageViews)
		{
			vkDestroyImageView(m_Device, imageView, nullptr);
		}		
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_Device, m_UniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
	}

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		
		CleanupSwapchain();

		vkDestroySampler(m_Device, m_TextureSampler, nullptr);
		vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
		vkDestroyImage(m_Device, m_TextureImage, nullptr);
		vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);

		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

		vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
		vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);

		vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
		vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InFlightFence[i], nullptr);
		}
		vkDestroyCommandPool(m_Device, m_CommandPoolTransfer, nullptr);
		vkDestroyCommandPool(m_Device, m_CommandPoolGraphics, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		if (m_EnableValidationLayers)
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_WindowHandle);
		glfwTerminate();
	}

	void VulkanContext::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();



	}

	void VulkanContext::DrawFrame()
	{
		vkWaitForFences(m_Device, 1, &m_InFlightFence[m_CurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX /*disables timeout*/, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

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
			vkWaitForFences(m_Device, 1, &m_InFlightFence[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		}
		else // mark the image as now being used now by this frame
		{
			m_ImagesInFlight[m_CurrentFrame] = m_InFlightFence[m_CurrentFrame];
		}

		UpdateUniformBuffer(imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount	= 1;
		submitInfo.pWaitSemaphores		= waitSemaphores;
		submitInfo.pWaitDstStageMask	= waitStages;
		submitInfo.commandBufferCount	= 1;
		submitInfo.pCommandBuffers		= &m_CommandBuffersGraphics[imageIndex];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores	= signalSemaphores;

		vkResetFences(m_Device, 1, &m_InFlightFence[m_CurrentFrame]);

		VkResult resultQueueSubmit = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFence[m_CurrentFrame]);
		PX_CORE_ASSERT(resultQueueSubmit == VK_SUCCESS, "Failed to submit draw render buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount	= 1;
		presentInfo.pWaitSemaphores		= signalSemaphores;

		VkSwapchainKHR swapchains[] = { m_Swapchain };
		presentInfo.swapchainCount		= 1;
		presentInfo.pSwapchains			= swapchains;
		presentInfo.pImageIndices		= &imageIndex;
		presentInfo.pResults			= nullptr;		// optional

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
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

	void VulkanContext::CreateInstance()
	{
		if (m_EnableValidationLayers && !CheckValidationLayerSupport())
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
		if (m_EnableValidationLayers)
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

		VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan Instance");
	}

	
// Debug
	// move to VulkanDebug
	void VulkanContext::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);	

		VkResult result = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
	}
	// move to VulkanDebug
	void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

// Surface
	void VulkanContext::CreateSurface()
	{
		VkResult result = glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create window surface!");
	}


// Physical Device
	void VulkanContext::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		PX_CORE_ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices)
		{
			int score = RateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		//TODO: change to choose highest score instead of first over 0
		if(candidates.rbegin()->first > 0)
		{
			m_PhysicalDevice = candidates.rbegin()->second;
		}
		PX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find suitable GPU!");
	}

	

// Logical Device
	void VulkanContext::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { 
									indices.GraphicsFamily.value(), 
									indices.PresentFamily.value(), 
									indices.TransferFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Here we specify the features we queried to with vkGetPhysicalDeviseFeatures in RateDeviceSuitability
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		if (m_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create logical device!");

		vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
		vkGetDeviceQueue(m_Device, indices.TransferFamily.value(), 0, &m_TransferQueue);
	}

// Swapchain
	// move to VulkanSwapchain
	void VulkanContext::CreateSwapchain()
	{
		SwapchainSupportDetails swapchainSupportDetails = QuerySwapchainSupport(m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupportDetails.Formats);
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupportDetails.PresentModes);
		VkExtent2D extent = ChooseSwapExtent(swapchainSupportDetails.Capabilities);

		uint32_t imageCount = swapchainSupportDetails.Capabilities.minImageCount + 1; // +1 to avoid waiting for driver to finish internal operations
		if (swapchainSupportDetails.Capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.Capabilities.maxImageCount)
			imageCount = swapchainSupportDetails.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.presentMode = presentMode;

		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;

		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { 
						indices.GraphicsFamily.value(), 
						indices.PresentFamily.value(), 
						indices.TransferFamily.value()
		};

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;		// optional
			createInfo.pQueueFamilyIndices = nullptr;	// optianal
		}

		createInfo.preTransform = swapchainSupportDetails.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create swapchain!");


		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
	}

	
	// move to VulkanSwapchain
	VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				return format;
		}
		return availableFormats[0];
	}

	// move to VulkanSwapchain
	VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& mode : availablePresentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // on mobile, FIFO_KHR is more likely to be used because of lower enegry consumption
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// move to VulkanSwapchain
	VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(m_WindowHandle, &width, &height);

			VkExtent2D actualExtend = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtend.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtend.width));
			actualExtend.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtend.height));

			return actualExtend;
		}
	}


// Image views

	VkImageView VulkanContext::CreateImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image		= image;
		createInfo.viewType		= VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format		= format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel	= 0;
		createInfo.subresourceRange.levelCount		= 1;
		createInfo.subresourceRange.baseArrayLayer	= 0;
		createInfo.subresourceRange.layerCount		= 1;

		VkImageView imageView;
		VkResult result = vkCreateImageView(m_Device, &createInfo, nullptr, &imageView);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create image view!");

		return imageView;
	}

	// move to VulkanSwapchain
	void VulkanContext::CreateImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			m_SwapchainImageViews[i] = CreateImageView(m_SwapchainImages[i], m_SwapchainImageFormat);
		}
	}


// Render pass
	// move to VulkanGraphicsPipeline or own VulkanRenderPass
	void VulkanContext::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format			= m_SwapchainImageFormat;
		colorAttachment.samples			= VK_SAMPLE_COUNT_1_BIT;		// format of swapchain images
		colorAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;	// load and store ops determine what to do with the data before rendering (apply to depth data)
		colorAttachment.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// we want the image to be ready for presentation in the swapchain, so final is present layout
		// Layout is importent to know, because the next operation the image is involved in needs that
		// initial is before render pass, final is the layout the imiga is automatically transitioned to after the render pass finishes

		// Subpasses  -> usefull for example to create a sequence of post processing effects
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment	= 0;		// index in attachment description array
		colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount	= 1;
		subpass.pColorAttachments		= &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;	// refers to before or after the render pass depending on it being specified on src or dst
		dependency.dstSubpass		= 0;						// index of subpass, dstSubpass must always be higher then src subpass unless on of them is VK_SUBPASS_EXTERNAL
		dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // operation to wait on
		dependency.srcAccessMask	= 0;
		dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount	= 1;
		renderPassInfo.pAttachments		= &colorAttachment;
		renderPassInfo.subpassCount		= 1;
		renderPassInfo.pSubpasses		= &subpass;
		renderPassInfo.dependencyCount	= 1;
		renderPassInfo.pDependencies	= &dependency;

		VkResult result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create render pass!");
	}


// Graphics Pipeline
	
	void VulkanContext::CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.binding				= 0;		// binding in the shader
		uboBinding.descriptorCount		= 1;		// set could be an array -> count is number of elements
		uboBinding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.pImmutableSamplers	= nullptr;	// optional ->  for image sampling

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding			= 1;		// binding in the shader
		samplerLayoutBinding.descriptorCount	= 1;		// set could be an array -> count is number of elements
		samplerLayoutBinding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;	// optional ->  for image sampling

		std::array< VkDescriptorSetLayoutBinding, 2> bindings{ uboBinding , samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount			= static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings			= bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor set layout!");
	}

	// move to VulkanGraphicsPipeline
	void VulkanContext::CreateGraphicsPipeline()
	{
		auto vertexShaderCode = ReadFile("assets/shaders/vert.spv");
		auto fragmentShaderCode = ReadFile("assets/shaders/frag.spv");
		PX_CORE_INFO("VertexCode size: '{0}'; Fragmentcode size: '{1}'", vertexShaderCode.size(), fragmentShaderCode.size());

		VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderInfo{};
		vertexShaderInfo.sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderInfo.stage		= VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderInfo.module		= vertexShaderModule;
		vertexShaderInfo.pName		= "main";						//entry point

		VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
		fragmentShaderInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderInfo.stage	= VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderInfo.module	= fragmentShaderModule;
		fragmentShaderInfo.pName	= "main";						//entry point

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderInfo, fragmentShaderInfo };


		// Describes Vertex data layout
		auto bindingDescription = VertexData::getBindingDescription();
		auto attributeDescriptions = VertexData::getAttributeDescriptions();


		VkPipelineVertexInputStateCreateInfo vertexStateInfo{};									
		vertexStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexStateInfo.vertexBindingDescriptionCount		= 1;
		vertexStateInfo.pVertexBindingDescriptions			= &bindingDescription;
		vertexStateInfo.vertexAttributeDescriptionCount		= static_cast<uint32_t>(attributeDescriptions.size());
		vertexStateInfo.pVertexAttributeDescriptions		= attributeDescriptions.data();


		// Kind of geometry and (primitive restart?)
		VkPipelineInputAssemblyStateCreateInfo assemblyStateInfo{};
		assemblyStateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assemblyStateInfo.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assemblyStateInfo.primitiveRestartEnable	= VK_FALSE;


		// Viewports and scissors
		VkViewport viewport;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;
		viewport.width		= (float)m_SwapchainExtent.width;
		viewport.height		= (float)m_SwapchainExtent.height;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapchainExtent;

		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount		= 1;
		viewportStateInfo.pViewports		= &viewport;
		viewportStateInfo.scissorCount		= 1;
		viewportStateInfo.pScissors			= &scissor;


		// Rasterizer -> rasterization, face culling, depth testing, scissoring, wire frame rendering
		VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
		rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateInfo.depthClampEnable			= VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
		rasterizationStateInfo.rasterizerDiscardEnable	= VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
		rasterizationStateInfo.polygonMode				= VK_POLYGON_MODE_FILL;					// determines if filled, lines or point shall be rendered
		rasterizationStateInfo.lineWidth				= 1.0f;									// thicker then 1.0f reuires wideLine GPU feature
		rasterizationStateInfo.cullMode					= VK_CULL_MODE_BACK_BIT;				
		rasterizationStateInfo.frontFace				= VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateInfo.depthBiasEnable			= VK_FALSE;


		// Multisampling   -> one way to anti-aliasing			(enabling also requires a GPU feature)
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		multisampleInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable		= VK_FALSE;
		multisampleInfo.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.minSampleShading		= 1.0f;		//	optional
		multisampleInfo.pSampleMask				= nullptr;	//	optional
		multisampleInfo.alphaToCoverageEnable	= VK_FALSE;	//	optional
		multisampleInfo.alphaToOneEnable		= VK_FALSE;	//	optional


		// Depth buffer and stencil testing
		//VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};


		//Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // conficuration per attached framebuffer
		colorBlendAttachment.colorWriteMask			= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable			= VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor	= VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstColorBlendFactor	= VK_BLEND_FACTOR_ZERO;		// optional
		colorBlendAttachment.colorBlendOp			= VK_BLEND_OP_ADD;			// optional
		colorBlendAttachment.srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;		// optional
		colorBlendAttachment.alphaBlendOp			= VK_BLEND_OP_ADD;			// optional

		/* in pseudo code, this happens while blending
		if (blendEnable)
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
		else 
			finalColor = newColor;
		finalColor = finalColor & colorWriteMask;		
		*/

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
		colorBlendStateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateInfo.logicOpEnable		= VK_FALSE;
		colorBlendStateInfo.logicOp				= VK_LOGIC_OP_COPY;
		colorBlendStateInfo.attachmentCount		= 1;
		colorBlendStateInfo.pAttachments		= &colorBlendAttachment;
		colorBlendStateInfo.blendConstants[0]	= 0.0f;
		colorBlendStateInfo.blendConstants[1]	= 0.0f;
		colorBlendStateInfo.blendConstants[2]	= 0.0f;
		colorBlendStateInfo.blendConstants[3]	= 0.0f;


		// Dynamic state  -> allows to modify some of the states above during runtime without recreating the pipeline, such as blend constants, line width and viewport
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{}; // with no dynamic state, just pass in a nullptr later on
		dynamicStateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount	= 2;
		dynamicStateInfo.pDynamicStates		= dynamicStates;


		// Pipeline layouts  -> commonly used to pass transformation matrices or texture samplers to the shader
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount			= 1;		// optional
		layoutInfo.pSetLayouts				= &m_DescriptorSetLayout;	// optional
		layoutInfo.pushConstantRangeCount	= 0;		// optional
		layoutInfo.pPushConstantRanges		= nullptr;	// optional

		VkResult resultPipelineLayout = vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &m_PipelineLayout);
		PX_CORE_ASSERT(resultPipelineLayout == VK_SUCCESS, "Failed to create pipeline layout!");


		// The Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount				= 2;
		pipelineInfo.pStages				= shaderStages;
		pipelineInfo.pVertexInputState		= &vertexStateInfo;
		pipelineInfo.pInputAssemblyState	= &assemblyStateInfo;
		pipelineInfo.pViewportState			= &viewportStateInfo;
		pipelineInfo.pRasterizationState	= &rasterizationStateInfo;
		pipelineInfo.pMultisampleState		= &multisampleInfo;
		pipelineInfo.pDepthStencilState		= nullptr;
		pipelineInfo.pColorBlendState		= &colorBlendStateInfo;
		pipelineInfo.pDynamicState			= nullptr;

		pipelineInfo.layout					= m_PipelineLayout;
		pipelineInfo.renderPass				= m_RenderPass;
		pipelineInfo.subpass				= 0; // index

		pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;	// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT is specified under flags in VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineIndex		= -1;
		
		VkResult result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Graphics pipeline!");
		
		vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
	}

	// move to VulkanGraphicsPipeline
	VkShaderModule VulkanContext::CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize		= code.size();
		createInfo.pCode		= reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;

		VkResult result = vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create shader module!");

		return shaderModule;
	}

	// move to VulkanUtil or completelz out and replace it with something else *subject to change with spirvcross(
	std::vector<char> VulkanContext::ReadFile(const std::string& filepath)
	{
		std::ifstream stream(filepath, std::ios::ate | std::ios::binary);
		bool isOpen = stream.is_open();
		PX_CORE_ASSERT(isOpen, "Failed to open file!");

		size_t fileSize = (size_t)stream.tellg();
		std::vector<char> buffer(fileSize);

		stream.seekg(0);
		stream.read(buffer.data(), fileSize);

		return buffer;
	}


// Framebuffers
	// move to VulkanFramebuffer
	void VulkanContext::CreateFramebuffers()
	{
		m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());
		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			VkImageView attachments[] = { m_SwapchainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass		= m_RenderPass;			// needs to be compatible (roughly, use the same number and type of attachments)
			framebufferInfo.attachmentCount = 1;					// The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
			framebufferInfo.pAttachments	= attachments;
			framebufferInfo.width			= m_SwapchainExtent.width;
			framebufferInfo.height			= m_SwapchainExtent.height;
			framebufferInfo.layers			= 1;					// number of layers in image array

			VkResult result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]);
			PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create framebuffer!");
		}
	}

// Command pools
	// move to VulkanCommandBuffer, but needs more research on what this exactlz does and how to use
	void VulkanContext::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilies = FindQueueFamilies(m_PhysicalDevice);

		VkCommandPoolCreateInfo commandPoolGraphicsInfo{};
		commandPoolGraphicsInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolGraphicsInfo.queueFamilyIndex	= queueFamilies.GraphicsFamily.value();
		commandPoolGraphicsInfo.flags				= 0;		// optional

		VkCommandPoolCreateInfo commandPoolTransferInfo{};
		commandPoolTransferInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolTransferInfo.queueFamilyIndex	= queueFamilies.TransferFamily.value();
		commandPoolTransferInfo.flags				= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		VkResult result = vkCreateCommandPool(m_Device, &commandPoolGraphicsInfo, nullptr, &m_CommandPoolGraphics);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create graphics command pool!");

		result = vkCreateCommandPool(m_Device, &commandPoolTransferInfo, nullptr, &m_CommandPoolTransfer);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create transfer command pool!");
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

		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_Device, stagingBufferMemory);

		stbi_image_free(pixels);

		CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

		TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	void VulkanContext::CreateTextureImageView()
	{
		m_TextureImageView = CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB);

	}

	void VulkanContext::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& memory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.mipLevels = 1;
		imageInfo.format = format;	// may not be supported by all hardware
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;	// used as destination from the buffer to image, and the shader needs to sample from it
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// exclusive means it will only be used by one queue family (graphics and thgerefore also transfer possible)
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;	// related to multi sampling -> in attachments
		imageInfo.flags = 0;	// optional, can be used for sparse textures, in "D vfor examples for voxel terrain, to avoid using memory with AIR

		VkResult result = vkCreateImage(m_Device, &imageInfo, nullptr, &image);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Texture Image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

		VkMemoryAllocateInfo memoryInfo{};
		memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryInfo.allocationSize = memRequirements.size;
		memoryInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		VkResult resultMemory = vkAllocateMemory(m_Device, &memoryInfo, nullptr, &memory);
		PX_CORE_ASSERT(resultMemory == VK_SUCCESS, "Failed to allocate texture image memory!");

		vkBindImageMemory(m_Device, image, memory, 0);
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
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
		samplerInfo.maxAnisotropy			= properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor				= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable			= VK_TRUE ;
		samplerInfo.compareOp				= VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias				= 0.0f;
		samplerInfo.maxLod					= 0.0f;
		samplerInfo.minLod					= 0.0f;

		VkResult result = vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create texture sampler!");
	}

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
		barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
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
		else
		{
			PX_CORE_ASSERT(false, "Unsupported layout transition!");
		}
			
		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		EndSingleTimeCommands(commandBuffer, m_GraphicsQueue, m_CommandPoolGraphics);
	}

	void VulkanContext::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_CommandPoolGraphics);

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

		EndSingleTimeCommands(commandBuffer, m_GraphicsQueue, m_CommandPoolGraphics);
	}

	void VulkanContext::CreateVertexBuffer()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = {
			indices.GraphicsFamily.value(),
			indices.TransferFamily.value()
		};
		uint32_t indiceSize = static_cast<uint32_t>(sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]));

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		
		VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_Device, stagingBufferMemory);
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer,
			m_VertexBufferMemory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);
		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	void VulkanContext::CreateIndexBuffer()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = {
			indices.GraphicsFamily.value(),
			indices.TransferFamily.value()
		};
		uint32_t indiceSize = static_cast<uint32_t>(sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]));

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VkDeviceSize bufferSize = sizeof(uint16_t) * m_Indices.size();
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Indices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_Device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer,
			m_IndexBufferMemory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	void VulkanContext::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
		VkBuffer& buffer, VkDeviceMemory& memory, uint32_t familyIndexCount, uint32_t* familyIndices, VkSharingMode sharingMode)
	{
		VkBufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.size = size;
		vertexBufferInfo.usage = usage;
		vertexBufferInfo.sharingMode = sharingMode;
		vertexBufferInfo.queueFamilyIndexCount = familyIndexCount;
		vertexBufferInfo.pQueueFamilyIndices = familyIndices;

		VkResult resultBuffer = vkCreateBuffer(m_Device, &vertexBufferInfo, nullptr, &buffer);
		PX_CORE_ASSERT(resultBuffer == VK_SUCCESS, "Failed to create vertex buffer!");


		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_Device, buffer, &requirements);

		VkMemoryAllocateInfo memoryInfo{};
		memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryInfo.allocationSize = requirements.size;
		memoryInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, properties);

		VkResult resultMemory = vkAllocateMemory(m_Device, &memoryInfo, nullptr, &memory);
		PX_CORE_ASSERT(resultMemory == VK_SUCCESS, "Failed to allocate vertex buffer memory!");

		vkBindBufferMemory(m_Device, buffer, memory, 0);
	}

	void VulkanContext::CreateUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(m_SwapchainImages.size());
		m_UniformBuffersMemory.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);
		}
	}

	void VulkanContext::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_CommandPoolTransfer);

		VkBufferCopy copyRegion{};
		copyRegion.size	= size;

		vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer, m_TransferQueue, m_CommandPoolTransfer);
	}

	uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertiyFlags)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			// iterates over the properties and checks if the bit of a flag is set to 1
			if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertiyFlags) == propertiyFlags)
			{
				return i;
			}
		}
		PX_CORE_ASSERT(false, "Failed to find suitable memory type!");
	}

	void VulkanContext::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount	= static_cast<uint32_t>(m_SwapchainImages.size());
		poolSizes[1].type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount	= static_cast<uint32_t>(m_SwapchainImages.size());
		
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount		= static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes			= poolSizes.data();
		poolInfo.maxSets			= static_cast<uint32_t>(m_SwapchainImages.size());

		VkResult result = vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool!");
	}

	void VulkanContext::CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(m_SwapchainImages.size(), m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool		= m_DescriptorPool;
		allocInfo.descriptorSetCount	= static_cast<uint32_t>(m_SwapchainImages.size());
		allocInfo.pSetLayouts			= layouts.data();

		m_DescriptorSets.resize(m_SwapchainImages.size());
		VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data());
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor sets!");

		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer	= m_UniformBuffers[i];
			bufferInfo.offset	= 0;
			bufferInfo.range	= sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView		= m_TextureImageView;
			imageInfo.sampler		= m_TextureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet				= m_DescriptorSets[i];
			descriptorWrites[0].dstBinding			= 0;		// again, binding in the shader
			descriptorWrites[0].dstArrayElement		= 0;		// descriptors can be arrays
			descriptorWrites[0].descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount		= 1;		// how many elements in the array
			descriptorWrites[0].pBufferInfo			= &bufferInfo;
			descriptorWrites[0].pImageInfo			= nullptr;	// optional image data

			descriptorWrites[1].sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet				= m_DescriptorSets[i];
			descriptorWrites[1].dstBinding			= 1;		// again, binding in the shader
			descriptorWrites[1].dstArrayElement		= 0;		// descriptors can be arrays
			descriptorWrites[1].descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount		= 1;		// how many elements in the array
			descriptorWrites[1].pBufferInfo			= &bufferInfo;
			descriptorWrites[1].pImageInfo			= &imageInfo;	// optional image data

			vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
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
		vkAllocateCommandBuffers(m_Device, &bufferAllocInfo, &commandBuffer);

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
		VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit copy buffer!");
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(m_Device, commandPool, 1, &commandBuffer);
	}

	void VulkanContext::CreateCommandBuffers()
	{
		m_CommandBuffersGraphics.resize(m_SwapchainFramebuffers.size());


		VkCommandBufferAllocateInfo allocInfoGraphics{};
		allocInfoGraphics.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfoGraphics.commandPool			= m_CommandPoolGraphics;
		allocInfoGraphics.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		allocInfoGraphics.commandBufferCount	= (uint32_t)m_CommandBuffersGraphics.size();

		VkResult result = vkAllocateCommandBuffers(m_Device, &allocInfoGraphics, m_CommandBuffersGraphics.data());
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create command buffers!");

		for (size_t i = 0; i < m_CommandBuffersGraphics.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags				= 0;		// optional
			beginInfo.pInheritanceInfo	= nullptr;	// optional, only used if buffer is secondary

			VkResult resultCommandBufferBegin = vkBeginCommandBuffer(m_CommandBuffersGraphics[i], &beginInfo);
			PX_CORE_ASSERT(resultCommandBufferBegin == VK_SUCCESS, "Failed to begin recording command buffer!");

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = m_SwapchainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_SwapchainExtent;
			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(m_CommandBuffersGraphics[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(m_CommandBuffersGraphics[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			VkBuffer vertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffersGraphics[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(m_CommandBuffersGraphics[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(m_CommandBuffersGraphics[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(m_CommandBuffersGraphics[i], static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
			vkCmdEndRenderPass(m_CommandBuffersGraphics[i]);

			VkResult resultCommandBufferEnd = vkEndCommandBuffer(m_CommandBuffersGraphics[i]);
			PX_CORE_ASSERT(resultCommandBufferEnd == VK_SUCCESS, "Failed to record graphics command buffer!");
		}
	}

// Semaphores
	// move to VulkanUtils or VulkanSyncObjects
	void VulkanContext::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFence.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImagesInFlight.resize(m_SwapchainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo createSemaphoreInfo{};
		createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			bool result = false;
			if ((vkCreateSemaphore(m_Device, &createSemaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS)
				&& (vkCreateSemaphore(m_Device, &createSemaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS)
				&& (vkCreateFence(m_Device, &createFenceInfo, nullptr, &m_InFlightFence[i]) == VK_SUCCESS))
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

// Debugging Callback
	// move to VulkanUtils or somewhere else...
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData)
	{
		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			{
				PX_CORE_INFO("VK-Debug Callback: '{0}'", callbackData->pMessage);
				break;
			}
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			{
				PX_CORE_WARN("VK-Debug Callback: '{0}'", callbackData->pMessage);
				break;
			}
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			{
				PX_CORE_ERROR("VK-Debug Callback: '{0}'", callbackData->pMessage);
				break;
			}
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			{
				PX_CORE_TRACE("VK-Debug Callback: '{0}'", callbackData->pMessage);
				break;
			}
		}
		return VK_FALSE;
	}


	int VulkanContext::RateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		PX_CORE_INFO("Device-Name: {0}", deviceProperties.deviceName);
		if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			PX_CORE_INFO("Device-type: Discrete GPU");
		PX_CORE_INFO("API-Version: {0}.{1}.{2}.{3}", VK_API_VERSION_VARIANT(deviceProperties.apiVersion), VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));


		int score = 0;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)		// Dedicated GPU
			score = 1000;
		score += deviceProperties.limits.maxImageDimension2D;							// Maximum Supported Textures
		//if (!deviceFeatures.geometryShader)											// Geometry Shader (needed in this case)
		//	return 0;

		bool extensionSupported = CheckDeviceExtensionSupport(device);
		bool swapchainAdequat = extensionSupported ? QuerySwapchainSupport(device).IsAdequat() : false;


		if (!FindQueueFamilies(device).IsComplete() && extensionSupported && swapchainAdequat && supportedFeatures.samplerAnisotropy)
		{
			PX_CORE_WARN("There are features missing!");
			return 0;
		}
		return score;
	}

	// move to VulkanUtils
	bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> aExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, aExtensions.data());

		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& aExtension : aExtensions)
		{
			requiredExtensions.erase(aExtension.extensionName);
		}

		return requiredExtensions.empty();
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

		if (m_EnableValidationLayers)
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

	// QuerySwapchainSupport
	SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkPhysicalDevice device)
	{
		SwapchainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	// FindQueuFamilies
	QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, &queueFamilies[0]);

		int i = 0;
		bool presentFound = false;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
			if (presentSupport && !presentFound)
			{
				indices.PresentFamily = i;
				presentFound = true;
			}

			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				indices.TransferFamily = i;
				PX_CORE_TRACE("transfer Queue Family index : '{0}'", indices.TransferFamily.value());
			}

			if (indices.IsComplete() && indices.HasTransfer())
			{
				PX_CORE_INFO("Present, Graphics and Transfer index set to: \n Present Family : '{0}' \n Graphics Family: '{1}' \n Transfer Family: '{2}'", indices.PresentFamily.value(), indices.GraphicsFamily.value(), indices.TransferFamily.value());
				break;
			}
			i++;
		}
		return indices;
	}

	void VulkanContext::UpdateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.ProjectionMatrix = glm::perspective(glm::radians(45.0f), m_SwapchainExtent.width / (float) m_SwapchainExtent.height, 0.1f, 10.0f);
		ubo.ProjectionMatrix[1][1] *= -1; //flips Y

		void* data;
		vkMapMemory(m_Device, m_UniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_Device, m_UniformBuffersMemory[currentImage]);

		// pushconstants are more efficient for small packages of data;
	}
}