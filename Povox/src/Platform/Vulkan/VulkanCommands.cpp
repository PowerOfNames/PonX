#include "pxpch.h"
#include "VulkanCommands.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"

namespace Povox {

// ========== Commands ==========
	void VulkanCommands::CopyBuffer(const VulkanCoreObjects& core, UploadContext& uploadContext, VkBuffer src, VkBuffer dst, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = VulkanCommandBuffer::BeginSingleTimeCommands(core.Device, uploadContext.CmdPoolTrsf);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

		VulkanCommandBuffer::EndSingleTimeCommands(core.Device, commandBuffer, core.QueueFamily.TransferQueue, uploadContext.CmdPoolTrsf, uploadContext.Fence);
	}

	void VulkanCommands::CopyImage(const VulkanCoreObjects& core, UploadContext& uploadContext, VkImage src, VkImage dst, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = VulkanCommandBuffer::BeginSingleTimeCommands(core.Device, uploadContext.CmdPoolTrsf);

		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = width;
		imageCopyRegion.extent.height = height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(commandBuffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

		VulkanCommandBuffer::EndSingleTimeCommands(core.Device, commandBuffer, core.QueueFamily.TransferQueue, uploadContext.CmdPoolTrsf, uploadContext.Fence);
	}

	void VulkanCommands::TransitionImageLayout(const VulkanCoreObjects& core, UploadContext& uploadContext, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = VulkanCommandBuffer::BeginSingleTimeCommands(core.Device, uploadContext.CmdPoolGfx);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (HasStencilComponent(format))
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else
		{
			PX_CORE_ASSERT(false, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VulkanCommandBuffer::EndSingleTimeCommands(core.Device, commandBuffer, core.QueueFamily.GraphicsQueue, uploadContext.CmdPoolGfx, uploadContext.Fence);
	}

	void VulkanCommands::CopyBufferToImage(const VulkanCoreObjects& core, UploadContext& uploadContext, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = VulkanCommandBuffer::BeginSingleTimeCommands(core.Device, uploadContext.CmdPoolTrsf);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferImageHeight = 0;
		region.bufferRowLength = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		
		VulkanCommandBuffer::EndSingleTimeCommands(core.Device, commandBuffer, core.QueueFamily.TransferQueue, uploadContext.CmdPoolTrsf, uploadContext.Fence);
	}

	void VulkanCommands::CopyImageToBuffer(const VulkanCoreObjects& core, UploadContext& uploadContext, VkImage image, VkImageLayout imageLayout, VkBuffer buffer, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = VulkanCommandBuffer::BeginSingleTimeCommands(core.Device, uploadContext.CmdPoolTrsf);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferImageHeight = 0;
		region.bufferRowLength = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		
		vkCmdCopyImageToBuffer(commandBuffer, image, imageLayout, buffer, 1, &region);
		VulkanCommandBuffer::EndSingleTimeCommands(core.Device, commandBuffer, core.QueueFamily.TransferQueue, uploadContext.CmdPoolTrsf, uploadContext.Fence);
	}


// ========== CommanPool ==========
	VkCommandPool VulkanCommandPool::Create(const VulkanCoreObjects& core, VkCommandPoolCreateInfo commandPoolInfo)
	{
		VkCommandPool pool;
		PX_CORE_VK_ASSERT(vkCreateCommandPool(core.Device, &commandPoolInfo, nullptr, &pool), VK_SUCCESS, "Failed to create graphics command pool!");
		return pool;
	}

	void VulkanCommandPool::Destroy(VkDevice device, VkCommandPool& pool)
	{
		vkDestroyCommandPool(device, pool, nullptr);
	}


// ========== CommandBuffer ==========
	VkCommandBuffer VulkanCommandBuffer::Create(VkDevice device, VkCommandPool& commandPool, VkCommandBufferAllocateInfo bufferInfo)
	{
		VkCommandBuffer buffer;
		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &bufferInfo, &buffer), VK_SUCCESS, "Failed to create CommandBuffer!");
		return buffer;
	}

	VkCommandBuffer VulkanCommandBuffer::BeginSingleTimeCommands(VkDevice device, VkCommandPool& commandPool)
	{
		VkCommandBufferAllocateInfo bufferAllocInfo{};
		bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocInfo.commandPool = commandPool;
		bufferAllocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &bufferAllocInfo, &commandBuffer);

		VkCommandBufferBeginInfo bufferBeginInfo{};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo);

		return commandBuffer;
	}

	void VulkanCommandBuffer::EndSingleTimeCommands(VkDevice device, VkCommandBuffer& commandBuffer, VkQueue queue, VkCommandPool& commandPool, VkFence fence)
	{
		vkEndCommandBuffer(commandBuffer);


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;


		// with fences I could submit multiple transfers and wait for all of them -> enable for more possible optimization opportunities
		PX_CORE_VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence), VK_SUCCESS, "Failed to submit copy buffer!");
		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &fence);

		vkResetCommandPool(device, commandPool, 0);
	}

}
