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

		// Here we specify the features we queried to with vkGetPhysicalDeviceFeatures in RateDeviceSuitability
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;	//check if supported

		VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures{};
		shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
		shaderDrawParametersFeatures.pNext = nullptr;
		shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.pNext = &shaderDrawParametersFeatures;

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
		PX_CORE_TRACE("Logical Device has been created!");


		vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_QueueFamilies.GraphicsQueue);
		vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_QueueFamilies.PresentQueue);
		vkGetDeviceQueue(m_Device, indices.TransferFamily.value(), 0, &m_QueueFamilies.TransferQueue);
		PX_CORE_TRACE("Queues have been set up!");
	}

	VkPhysicalDeviceProperties VulkanDevice::GetPhysicalDeviceProperties()
	{
		VkPhysicalDeviceProperties properties;
		PX_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Physical device has not been picked yet!");
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

		
		return properties;
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
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);

		PX_CORE_TRACE("Physical Device '{0}' has been picked!", deviceProperties.deviceName);
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

	QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, &queueFamilies[0]);

		int i = 0;
		bool presentFound = false;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, VulkanSwapchain::GetSurface(), &presentSupport);
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
}
