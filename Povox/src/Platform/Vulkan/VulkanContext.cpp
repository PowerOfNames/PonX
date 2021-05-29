#include "pxpch.h"
#include "VulkanContext.h"

#include "VulkanLookups.h"

#include <cstdint>
#include <fstream>


namespace Povox {

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		PX_CORE_ASSERT(windowHandle, "Window handle is null");
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
		CreateGraphicsPipeline();
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

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		for (auto imageView : m_SwapchainImageViews)
		{
			vkDestroyImageView(m_Device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
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


	void VulkanContext::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);	

		PX_CORE_ASSERT(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) == VK_SUCCESS, "Failed to set up debug messenger!");
	}


// Surface
	void VulkanContext::CreateSurface()
	{
		PX_CORE_ASSERT(glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface) == VK_SUCCESS, "Failed to create window surface!");
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

		if(candidates.rbegin()->first > 0)
		{
			m_PhysicalDevice = candidates.rbegin()->second;
		}
		PX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find suitable GPU!");
	}

	// Will grow as more features are implemented
	int VulkanContext::RateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)		// Dedicated GPU
			score = 1000;
		score += deviceProperties.limits.maxImageDimension2D;							// Maximum Supported Textures
		//if (!deviceFeatures.geometryShader)											// Geometry Shader (needed in this case)
		//	return 0;

		bool extensionSupported = CheckDeviceExtensionSupport(device);
		bool swapchainAdequat = extensionSupported ? QuerySwapchainSupport(device).IsAdequat() : false;
		
		if (!FindQueueFamilies(device).IsComplete() && extensionSupported && swapchainAdequat)
		{
			PX_CORE_WARN("There is no graphics family");
			return 0;
		}
		return score;
	}

	QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamily = i;
			
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
			if (presentSupport)
				indices.PresentFamily = i;

			if (indices.IsComplete())
				break;
			i++;
		}
		return indices;
	}


// Logical Device
	void VulkanContext::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex	= queueFamily;
			queueCreateInfo.queueCount			= 1;
			queueCreateInfo.pQueuePriorities	= &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);

		}

		// Here we specify the features we queried to with vkGetPhysicalDeviseFeatures in RateDeviceSuitability
		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount		= static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos		= queueCreateInfos.data();
		createInfo.pEnabledFeatures			= &deviceFeatures;
		createInfo.enabledExtensionCount	= static_cast<uint32_t>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames	= m_DeviceExtensions.data();


		if (m_EnableValidationLayers)
		{
			createInfo.enabledLayerCount	= static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames	= m_ValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount	= 0;
		}

		PX_CORE_ASSERT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) == VK_SUCCESS, "Failed to create logical device!");

		vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
	}


// Swapchain
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
		createInfo.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface			= m_Surface;		
		createInfo.minImageCount	= imageCount;
		createInfo.imageFormat		= surfaceFormat.format;
		m_SwapchainImageFormat		= surfaceFormat.format;
		createInfo.imageColorSpace	= surfaceFormat.colorSpace;
		createInfo.imageExtent		= extent;
		m_SwapchainExtent			= extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.presentMode		= presentMode;

		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { indices.GraphicsFamily.value(), indices.PresentFamily.value() };

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode			= VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount	= 2;
			createInfo.pQueueFamilyIndices		= queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount	= 0;		// optional
			createInfo.pQueueFamilyIndices		= nullptr;	// optianal
		}

		createInfo.preTransform		= swapchainSupportDetails.Capabilities.currentTransform;
		createInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode		= presentMode;
		createInfo.clipped			= VK_TRUE;
		createInfo.oldSwapchain		= VK_NULL_HANDLE;

		PX_CORE_ASSERT(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain) == VK_SUCCESS, "Failed to create swapchain!");


		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
	}

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

	VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				return format;
		}
		return availableFormats[0];
	}

	VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& mode : availablePresentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // on mobile, FIFO_KHR is more likely to be used because of lower enegry consumption
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

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

	void VulkanContext::CreateImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image			= m_SwapchainImages[i];
			createInfo.viewType			= VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format			= m_SwapchainImageFormat;

			createInfo.components.r		= VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g		= VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b		= VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a		= VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel	= 0;
			createInfo.subresourceRange.levelCount		= 1;
			createInfo.subresourceRange.baseArrayLayer	= 0;
			createInfo.subresourceRange.layerCount		= 1;

			PX_CORE_ASSERT(vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]) == VK_SUCCESS, "Failed to create swapchainimage views!");
		}
	}

	void VulkanContext::CreateGraphicsPipeline()
	{
		auto vertexShaderCode = ReadFile("assets/shaders/vert.spv");
		auto fragmentShaderCode = ReadFile("assets/shaders/frag.spv");
		PX_CORE_INFO("VertexCode size: '{0}'; Fragmentcode size: '{1}'", vertexShaderCode.size(), fragmentShaderCode.size());

		VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderInfo{};
		vertexShaderInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderInfo.stage			= VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderInfo.module			= vertexShaderModule;
		vertexShaderInfo.pName			= "main";						//entry point

		VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
		fragmentShaderInfo.sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderInfo.stage		= VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderInfo.module		= fragmentShaderModule;
		fragmentShaderInfo.pName		= "main";						//entry point

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderInfo, fragmentShaderInfo };


		// Describes Vertex data layout
		VkPipelineVertexInputStateCreateInfo vertexStateInfo{};									
		vertexStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexStateInfo.vertexBindingDescriptionCount			= 0;
		vertexStateInfo.pVertexBindingDescriptions				= nullptr;
		vertexStateInfo.vertexAttributeDescriptionCount			= 0;
		vertexStateInfo.pVertexAttributeDescriptions			= nullptr;


		// Kind of geometry and (primitive restart?)
		VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
		assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assemblyInfo.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		assemblyInfo.primitiveRestartEnable		= VK_FALSE;


		// Vieports and scissors
		VkViewport viewport;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;
		viewport.width		= (float)m_SwapchainExtent.width;
		viewport.height		= (float)m_SwapchainExtent.height;
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0.0f, 0.0f };
		scissor.extent = m_SwapchainExtent;

		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount		= 1;
		viewportInfo.pViewports			= &viewport;
		viewportInfo.scissorCount		= 1;
		viewportInfo.pScissors			= &scissor;


		// Rasterizer -> rasterization, face culling, depth testing, scissoring, wire frame rendering
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.depthClampEnable			= VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
		rasterizationInfo.rasterizerDiscardEnable	= VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
		rasterizationInfo.polygonMode				= VK_POLYGON_MODE_FILL;					// determines if filled, lines or point shall be rendered
		rasterizationInfo.lineWidth					= 1.0f;									// thicker then 1.0f reuires wideLine GPU feature
		rasterizationInfo.cullMode					= VK_CULL_MODE_BACK_BIT;				// 
		rasterizationInfo.frontFace					= VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.depthBiasEnable			= VK_FALSE;
		rasterizationInfo.depthBiasClamp			= 0.0f;		//	optional
		rasterizationInfo.depthBiasConstantFactor	= 0.0f;		//	optional
		rasterizationInfo.depthBiasSlopeFactor		= 0.0f;		//	optional


		// Multisampling   -> one way to anti-aliasing			(enabling also rewuires a GPU feature)
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		multisampleInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable			= VK_FALSE;
		multisampleInfo.rasterizationSamples		= VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.minSampleShading			= 1.0f;		//	optional
		multisampleInfo.pSampleMask					= nullptr;	//	optional
		multisampleInfo.alphaToCoverageEnable		= VK_FALSE;	//	optional
		multisampleInfo.alphaToOneEnable			= VK_FALSE;	//	optional


		// Depth buffer and stencil testing
		//VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};


		//Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // conficuration per attached framebuffer
		colorBlendAttachment.colorWriteMask				= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable				= VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor		= VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstColorBlendFactor		= VK_BLEND_FACTOR_ZERO;		// optional
		colorBlendAttachment.colorBlendOp				= VK_BLEND_OP_ADD;			// optional
		colorBlendAttachment.srcAlphaBlendFactor		= VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstAlphaBlendFactor		= VK_BLEND_FACTOR_ZERO;		// optional
		colorBlendAttachment.alphaBlendOp				= VK_BLEND_OP_ADD;			// optional

		/* in pseudo code, this happens while blending
		if (blendEnable)
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
		else 
			finalColor = newColor;
		finalColor = finalColor & colorWriteMask;		
		*/

		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		colorBlendInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.logicOpEnable		= VK_FALSE;
		colorBlendInfo.logicOp				= VK_LOGIC_OP_COPY;
		colorBlendInfo.attachmentCount		= 1;
		colorBlendInfo.pAttachments			= &colorBlendAttachment;
		colorBlendInfo.blendConstants[0]	= 0.0f;
		colorBlendInfo.blendConstants[1]	= 0.0f;
		colorBlendInfo.blendConstants[2]	= 0.0f;
		colorBlendInfo.blendConstants[3]	= 0.0f;


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
		layoutInfo.setLayoutCount			= 0;		// optional
		layoutInfo.pSetLayouts				= nullptr;	// optional
		layoutInfo.pushConstantRangeCount	= 0;		// optional
		layoutInfo.pPushConstantRanges		= nullptr;	// optional

		PX_CORE_ASSERT(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS, "Failed to create pipeline layout!");


		vkDestroyShaderModule(m_Device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, fragmentShaderModule, nullptr);
	}

	VkShaderModule VulkanContext::CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size;
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		PX_CORE_ASSERT(vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS, "Failed to create shader module!");

		return shaderModule;
	}

	void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	void VulkanContext::CheckRequiredExtensions(const char** glfwExtensions, uint32_t glfWExtensionsCount)
	{
		PX_CORE_INFO("Required Extensioncount: {0}", glfWExtensionsCount);

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		for (uint32_t i = 0; i < glfWExtensionsCount; i++)
		{
			bool extensionFound = false;
			PX_CORE_INFO("Required Extension #{0}: {1}", i, glfwExtensions[i]);
			uint32_t index = 1;
			for (const auto& extension : availableExtensions)
			{
				if (strcmp(glfwExtensions[i], extension.extensionName))
				{
					extensionFound = true;
					PX_CORE_INFO("Extension requested and included {0}", glfwExtensions[i]);
					break;
				}
			}
			if (!extensionFound)
				PX_CORE_WARN("Extension requested but not included: {0}", glfwExtensions[i]);
		}
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

	bool VulkanContext::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* vLayer : m_ValidationLayers)
		{
			PX_CORE_INFO("Validation layer: '{0}'", vLayer);
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				PX_CORE_INFO("available layers: '{0}'", layerProperties.layerName);

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

// File reading
	std::vector<char> VulkanContext::ReadFile(const std::string& filepath)
	{
		std::ifstream stream(filepath, std::ios::ate | std::ios::binary);

		PX_CORE_ASSERT(stream.is_open(), "Failed to open file!");

		size_t fileSize = (size_t)stream.tellg();
		std::vector<char> buffer(fileSize);

		stream.seekg(0);
		stream.read(buffer.data(), fileSize);

		return buffer;
	}


// Debugging Callback
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


}