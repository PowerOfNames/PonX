#pragma once

#include "VulkanUtility.h"
#include <vulkan/vulkan.h>

namespace Povox {

	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice();

		void Destroy();

		void PickPhysicalDevice(VkInstance instance,VkSurfaceKHR surface);
		void CreateLogicalDevice(VkSurfaceKHR surface, const std::vector<const char*> validationLayers);

		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


		VkDevice GetLogicalDevice() { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

		const QueueFamilies& GetQueueFamilies() const { return m_Queues; }

		void AddDeviceExtension(const char* extension) { m_DeviceExtensions.push_back(extension); }

	private:
		int RatePhysicalDevice(VkPhysicalDevice phgysicalDevice, VkSurfaceKHR surface);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device;

		QueueFamilies m_Queues;

		std::vector<const char*> m_DeviceExtensions = {};
	};

}