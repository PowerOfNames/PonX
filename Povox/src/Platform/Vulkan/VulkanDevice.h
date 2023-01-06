#pragma once
#include "VulkanUtilities.h"

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

		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
		
		void PickPhysicalDevice(const std::vector<const char*>& deviceExtensions);
		void CreateLogicalDevice(const std::vector<const char*>& deviceExtensions, const std::vector<const char*> validationLayers);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);
		inline const QueueFamilies& GetQueueFamilies() { return m_QueueFamilies; }

		VkPhysicalDeviceProperties GetPhysicalDeviceProperties();

		inline VkDevice GetVulkanDevice() { return m_Device; }
		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

	private:
		int RatePhysicalDevice(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice);
		bool CheckDeviceExtensionSupport(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		QueueFamilies m_QueueFamilies;

		PhysicalDeviceLimits m_PhysicalLimits;
	};

}
