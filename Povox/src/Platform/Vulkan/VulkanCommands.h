#pragma once

#include "VulkanDevice.h"
#include "VulkanUtility.h"

#include <vulkan/vulkan.h>

namespace Povox {


	class VulkanCommands
	{
	public:
		static void CopyBuffer(const VulkanCoreObjects& core, UploadContext& uploadContext, VkBuffer src, VkBuffer dst, VkDeviceSize size);
		static void CopyImage(const VulkanCoreObjects& core, UploadContext& uploadContext, VkImage src , VkImage dst, uint32_t width, uint32_t height);
		static void TransitionImageLayout(const VulkanCoreObjects& core, UploadContext& uploadContext, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		static void CopyBufferToImage(const VulkanCoreObjects& core, UploadContext& uploadContext, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		static void CopyImageToBuffer(const VulkanCoreObjects& core, UploadContext& uploadContext, VkImage image, VkImageLayout imageLayout, VkBuffer buffer, uint32_t width, uint32_t height);

	private:
		static bool HasStencilComponent(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}
	};

	class VulkanCommandPool
	{
	public:
		static VkCommandPool Create(const VulkanCoreObjects& core, VkCommandPoolCreateInfo commandPoolInfo);
		static void Destroy(VkDevice device, VkCommandPool& pool);

	};


	class VulkanCommandBuffer
	{
	public: 
		static VkCommandBuffer Create(VkDevice device, VkCommandPool& commandPool, VkCommandBufferAllocateInfo bufferInfo);

		static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool& commandPool);
		static void EndSingleTimeCommands(VkDevice device, VkCommandBuffer& commandBuffer, VkQueue queue, VkCommandPool& commandPool, VkFence fence);

	};

}
