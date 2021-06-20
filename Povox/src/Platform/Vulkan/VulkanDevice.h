#pragma once

#include <vulkan/vulkan.h>


namespace Povox {

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		bool IsAdequat() { return !Formats.empty() && !PresentModes.empty(); }
	};

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

	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice();

		void Destroy();

		void PickPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface);
		void CreateLogicalDevice(const VkSurfaceKHR& surface, const std::vector<const char*> validationLayers, bool enableValidationLayer);

		SwapchainSupportDetails QuerySwapchainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
		QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);


		const VkDevice& GetLogicalDevice() const { return m_Device; }
		VkDevice GetLogicalDevice() { return m_Device; }
		const VkPhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }

		const QueueFamilies& GetQueueFamilies() const { return m_Queues; }

		void AddDeviceExtension(const char* extension) { m_DeviceExtensions.push_back(extension); }

	private:
		int RatePhysicalDevice(const VkPhysicalDevice& phgysicalDevice, const VkSurfaceKHR& surface);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device;

		QueueFamilies m_Queues;

		std::vector<const char*> m_DeviceExtensions = {};
	};

}