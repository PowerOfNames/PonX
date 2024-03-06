#pragma once
#include "VulkanUtilities.h"

namespace Povox {

	struct QueueFamilies
	{
		int32_t GraphicsFamilyIndex = -1;
		int32_t PresentFamilyIndex = -1;					// Does support windows
		int32_t ComputeFamilyIndex = -1;
		int32_t TransferFamilyIndex = -1;					// not supported by all GPUs

		uint32_t UniqueQueueFamilies = 0;

		struct Queues
		{
			VkQueue GraphicsQueue;
			uint32_t GraphicsQueueCount;
			VkQueue PresentQueue;
			VkQueue ComputeQueue;
			VkQueue TransferQueue;
		} Queues;

		// TODO: check if ComputeSupport is mandatory or not
		bool IsComplete() { return (GraphicsFamilyIndex != -1) && (PresentFamilyIndex != -1); }
		bool HasTransfer() { return TransferFamilyIndex != -1; }
		bool HasCompute() { return ComputeFamilyIndex  != -1; }
		bool TransferExclusive() { return TransferFamilyIndex != GraphicsFamilyIndex; }
		bool FullyExclusive() { return TransferExclusive() && (ComputeFamilyIndex != GraphicsFamilyIndex) && (ComputeFamilyIndex != TransferFamilyIndex); }
	};

	

	struct PhysicalDeviceLimits
	{
		uint32_t MaxBoundDescriptorSets;
		uint32_t MinBufferAlign;

		// Compute
		struct MaxComputeWorkGoupCount
		{
			uint32_t X;
			uint32_t Y;
			uint32_t Z;
		}MaxComputeWorkGoupCount;
		uint32_t MaxComputeWorkGroupInvocations;
		struct MaxComputeWorkGroupSize
		{
			uint32_t X;
			uint32_t Y;
			uint32_t Z;
		}MaxComputeWorkGroupSize;

		float TimestampPeriod;

		bool HasDedicatedTransferQueue = false;
		bool HasDedicatedComputeQueue = false;


		VkPhysicalDeviceProperties Properties;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice() = default;
		~VulkanDevice();

		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice);
		
		void PickPhysicalDevice(const std::vector<const char*>& deviceExtensions);
		void CreateLogicalDevice(const std::vector<const char*>& deviceExtensions, const std::vector<const char*> validationLayers);

		inline const QueueFamilies& GetQueueFamilies() const { return m_QueueFamilies; }
		inline QueueFamilies& GetQueueFamilies() { return m_QueueFamilies; }

		inline VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return m_PhysicalLimits.Properties; }
		inline const PhysicalDeviceLimits& GetLimits() const { return m_PhysicalLimits; }

		inline VkDevice GetVulkanDevice() { return m_Device; }
		inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

	private:
		QueueFamilies FindQueueFamilies(VkPhysicalDevice physicalDevice);
		int RatePhysicalDevice(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice);
		PhysicalDeviceLimits QueryPhysicalDeviceLimits(VkPhysicalDevice physicalDevice);
		
		bool CheckDeviceExtensionSupport(const std::vector<const char*>& deviceExtensions, VkPhysicalDevice physicalDevice);

	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		PhysicalDeviceLimits m_PhysicalLimits;

		VkDevice m_Device = VK_NULL_HANDLE;
		QueueFamilies m_QueueFamilies;
	};

}
