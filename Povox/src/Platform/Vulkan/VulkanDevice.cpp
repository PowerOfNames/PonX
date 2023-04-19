#include "pxpch.h"
#include "VulkanDevice.h"

#include "Povox/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanSwapchain.h"


namespace Povox {

	VulkanDevice::~VulkanDevice()
	{
		vkDestroyDevice(m_Device, nullptr);
	}

	void VulkanDevice::CreateLogicalDevice(const std::vector<const char*>& deviceExtensions, const std::vector<const char*> validationLayers)
	{
		PX_CORE_INFO("VulkanDevice::CreateLogicalDevice: Starting creation...");

		m_QueueFamilies = FindQueueFamilies(m_PhysicalDevice);
		PX_CORE_TRACE("GraphicsQueueIndex: '{1}' | TransferQueueIndex '{2}' | Present'Queue'Index: '{0}'", m_QueueFamilies.PresentFamilyIndex, m_QueueFamilies.GraphicsFamilyIndex, m_QueueFamilies.TransferFamilyIndex);


		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(m_QueueFamilies.UniqueQueueFamilies);

		//These should be all the same if there is only one queueFamily
		std::set<uint32_t> queueFamilyIndices = {
									static_cast<uint32_t>(m_QueueFamilies.GraphicsFamilyIndex),
									static_cast<uint32_t>(m_QueueFamilies.PresentFamilyIndex),
									static_cast<uint32_t>(m_QueueFamilies.TransferFamilyIndex) };

		float queuePriority = 1.0f;
		for (uint32_t queueIndex : queueFamilyIndices)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueIndex;
			queueCreateInfo.queueCount = 1;
			if (m_QueueFamilies.UniqueQueueFamilies == 1 && queueIndex == m_QueueFamilies.GraphicsFamilyIndex)
				queueCreateInfo.queueCount = m_QueueFamilies.Queues.GraphicsQueueCount;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Here we specify the features we queried to with vkGetPhysicalDeviceFeatures in RateDeviceSuitability
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;
		deviceFeatures.pipelineStatisticsQuery = VK_TRUE;

		

		VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
		shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
		shaderDrawParametersFeatures.pNext = nullptr;
		shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;

		VkPhysicalDeviceHostQueryResetFeatures resetFeature{};
		resetFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
		resetFeature.pNext = &shaderDrawParametersFeatures;
		resetFeature.hostQueryReset = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.pNext = &resetFeature;

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		PX_CORE_VK_ASSERT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), VK_SUCCESS, "Failed to create logical device!");

		
		//TODO: Maybe get rid of the present queue altogether -> just querey for presentSupport and get one of them handles instead
		vkGetDeviceQueue(m_Device, m_QueueFamilies.GraphicsFamilyIndex, 0, &m_QueueFamilies.Queues.GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilies.PresentFamilyIndex, 0, &m_QueueFamilies.Queues.PresentQueue);
		if (m_QueueFamilies.TransferFamilyIndex == m_QueueFamilies.GraphicsFamilyIndex && m_QueueFamilies.Queues.GraphicsQueueCount > 1 )
			vkGetDeviceQueue(m_Device, m_QueueFamilies.TransferFamilyIndex, 1, &m_QueueFamilies.Queues.TransferQueue);
		else
			vkGetDeviceQueue(m_Device, m_QueueFamilies.TransferFamilyIndex, 0, &m_QueueFamilies.Queues.TransferQueue);



		PX_CORE_INFO("VulkanDevice::CreateLogicalDevice: Completed creation.");
	}

	int VulkanDevice::RatePhysicalDevice(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		PX_CORE_TRACE("Device-Name: {0}", deviceProperties.deviceName);
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			PX_CORE_TRACE("Device-type: Discrete GPU");
		PX_CORE_TRACE("API-Version: VulklanSDK {0}.{1}.{2}", VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));


		int score = 0;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)		// Dedicated GPU
			score = 1000;
		score += deviceProperties.limits.maxImageDimension2D;							// Maximum Supported Textures
		//if (!deviceFeatures.geometryShader)											// Geometry Shader (needed in this case)
		//	return 0;

		bool extensionSupported = CheckDeviceExtensionSupport(deviceExtensions, physicalDevice);
		bool swapchainAdequat = extensionSupported ? QuerySwapchainSupport(physicalDevice).IsAdequat() : false;


		if (!FindQueueFamilies(physicalDevice).IsComplete() && extensionSupported && swapchainAdequat && supportedFeatures.samplerAnisotropy)
		{
			PX_CORE_WARN("There are features missing!");
			return 0;
		}
		return score;
	}

	PhysicalDeviceLimits VulkanDevice::QueryPhysicalDeviceLimits(VkPhysicalDevice physicalDevice)
	{
		PhysicalDeviceLimits limits{};
		vkGetPhysicalDeviceProperties(physicalDevice, &limits.Properties);
		limits.MaxBoundDescriptorSets = limits.Properties.limits.maxBoundDescriptorSets;
		limits.MinBufferAlign = limits.Properties.limits.minUniformBufferOffsetAlignment;
		limits.TimestampPeriod = limits.Properties.limits.timestampPeriod;

		return limits;
	}

	void VulkanDevice::PickPhysicalDevice(const std::vector<const char*>& deviceExtensions)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(VulkanContext::GetInstance(), &deviceCount, nullptr);
		PX_CORE_ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(VulkanContext::GetInstance(), &deviceCount, devices.data());

		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices)
		{
			int score = RatePhysicalDevice(deviceExtensions, device);
			candidates.insert(std::make_pair(score, device));
		}
		//TODO: change to choose highest score instead of first over 0
		if (candidates.rbegin()->first > 0)
		{
			m_PhysicalDevice = candidates.rbegin()->second;
		}
		PX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find suitable GPU!");


		PX_CORE_TRACE("Physical Device '{0}' has been picked!", m_PhysicalLimits.Properties.deviceName);
		m_PhysicalLimits = QueryPhysicalDeviceLimits(m_PhysicalDevice);

		PX_CORE_INFO("");
		PX_CORE_INFO("---- Picked Phsical Device Limits ----");
		PX_CORE_INFO("MaxBoundDescriptorSets	: {0}", m_PhysicalLimits.MaxBoundDescriptorSets);
		PX_CORE_INFO("MinBufferAlignment		: {0}", m_PhysicalLimits.MinBufferAlign);
		PX_CORE_INFO("TimestampPeriod		: {0}", m_PhysicalLimits.TimestampPeriod);
		PX_CORE_INFO("--------------------------------------");
		PX_CORE_INFO("");
	}

	SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice physicalDevice)
	{
		SwapchainSupportDetails details;

		VkSurfaceKHR surface = VulkanSwapchain::GetSurface();
		PX_CORE_ASSERT(surface != VK_NULL_HANDLE, "Surface has not been instantiated yet!");

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.Formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.PresentModes.data());
		}

		return details;
	}

	bool VulkanDevice::CheckDeviceExtensionSupport(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> aExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, aExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& aExtension : aExtensions)
		{
			requiredExtensions.erase(aExtension.extensionName);
		}

		return requiredExtensions.empty();
	}

	//TODO: ComputeQueueQuery
	/**
	 * 1. Queries the supported QueueFamilyCount of physicalDevice
	 * 2. Checks for PresentSupport
	 * 3. Tries to find dedicated TransferQueue
	 * 4. Returns most fitting Index for TransferQueue if no dedicated found
	 */
	QueueFamilies VulkanDevice::FindQueueFamilies(VkPhysicalDevice physicalDevice)
	{
		QueueFamilies indices;

		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &indices.UniqueQueueFamilies, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(indices.UniqueQueueFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &indices.UniqueQueueFamilies, &queueFamilies[0]);

		int i = 0;
		bool presentFound = false;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.GraphicsFamilyIndex = i;
				indices.Queues.GraphicsQueueCount = queueFamilies[i].queueCount;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, VulkanSwapchain::GetSurface(), &presentSupport);
			if (presentSupport && !presentFound)
			{
				indices.PresentFamilyIndex = i;
				presentFound = true;
			}

			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				indices.TransferFamilyIndex = i;
				m_PhysicalLimits.HasDedicatedTransferQueue = true;
			}

			if (indices.IsComplete() && indices.HasTransfer())
			{
				break;
			}
			i++;
		}
		PX_CORE_ASSERT(presentFound, "No presentSupport on this device!");
		if (!m_PhysicalLimits.HasDedicatedTransferQueue)
		{
			indices.TransferFamilyIndex = indices.GraphicsFamilyIndex;
		}

		return indices;
	}
}
