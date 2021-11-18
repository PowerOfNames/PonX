#pragma once

#include "VulkanUtility.h"
#include <vulkan/vulkan.h>

namespace Povox {

	class VulkanDevice
	{
	public:
		static void PickPhysicalDevice(VulkanCoreObjects& core, const std::vector<const char*>& deviceExtensions);
		static void CreateLogicalDevice(VulkanCoreObjects& core, const std::vector<const char*>& deviceExtensions, const std::vector<const char*> validationLayers);

		static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		static SwapchainSupportDetails QuerySwapchainSupport(const VulkanCoreObjects& core) { return QuerySwapchainSupport(core.PhysicalDevice, core.Surface); };

		static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		static QueueFamilyIndices FindQueueFamilies(const VulkanCoreObjects& core) { return FindQueueFamilies(core.PhysicalDevice, core.Surface); };

		static VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice);

	private:
		static int RatePhysicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions);
		static bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions);
		
	};

}
