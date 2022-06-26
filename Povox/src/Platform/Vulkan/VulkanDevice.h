#pragma once

#include "VulkanUtility.h"
#include <vulkan/vulkan.h>

namespace Povox {

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;					// Does support windows
		std::optional<uint32_t> TransferFamily;					// not supported by all GPUs

		bool IsComplete() { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
		bool HasTransfer() { return TransferFamily.has_value(); }
	};

	struct QueueFamilies
	{
		VkQueue GraphicsQueue;
		VkQueue PresentQueue;
		VkQueue TransferQueue;
	};

	struct PhysicalDeviceLimits
	{
		uint32_t MaxBoundDescriptorSets;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice();

		SwapchainSupportDetails QuerySwapchainSupport();
		
		void PickPhysicalDevice(const std::vector<const char*>& deviceExtensions);
		void CreateLogicalDevice(const std::vector<const char*>& deviceExtensions, const std::vector<const char*> validationLayers);

		QueueFamilyIndices FindQueueFamilies();
		inline const QueueFamilies& GetQueueFamilies() { return m_QueueFamilies; }

		VkPhysicalDeviceProperties GetPhysicalDeviceProperties();

		inline VkDevice GetVulkanDevice() const { return m_Device; }
		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

	private:
		int RatePhysicalDevice(const std::vector<const char*>& deviceExtensions);
		bool CheckDeviceExtensionSupport(const std::vector<const char*>& deviceExtensions);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		QueueFamilies m_QueueFamilies;

		PhysicalDeviceLimits m_PhysicalLimits;
	};

}
