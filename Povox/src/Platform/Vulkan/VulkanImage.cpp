#include "pxpch.h"
#include "VulkanImage.h"

#include "VulkanDebug.h"
#include "VulkanInitializers.h"

#include "VulkanBuffer.h"
#include "VulkanCommands.h"

#include <stb_image.h>


namespace Povox {

	AllocatedImage VulkanImage::Create(VulkanCoreObjects& core, VkImageCreateInfo imageInfo, VmaMemoryUsage memoryUsage)
	{
		AllocatedImage newImage;

		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = memoryUsage;

		PX_CORE_VK_ASSERT(vmaCreateImage(core.Allocator, &imageInfo, &allocationInfo, &newImage.Image, &newImage.Allocation, nullptr), VK_SUCCESS, "Failed to create Image!");

		return newImage;
	}

	AllocatedImage VulkanImage::LoadFromFile(VulkanCoreObjects& core, UploadContext& uploadContext, const char* path, VkFormat format)
	{
		int width, height, channels;
		stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
		PX_CORE_ASSERT(pixels, "Failed to load texture!");
		VkDeviceSize imageSize = width * height * 4;

		PX_CORE_TRACE("Image width : '{0}', height '{1}'", width, height);

		AllocatedBuffer stagingBuffer = VulkanBuffer::Create(core, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		void* data;
		vmaMapMemory(core.Allocator, stagingBuffer.Allocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(core.Allocator, stagingBuffer.Allocation);

		stbi_image_free(pixels);

		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(width);
		extent.height = static_cast<uint32_t>(height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extent);

		AllocatedImage output;
		output = VulkanImage::Create(core, imageInfo);

		VulkanCommands::ImmidiateSubmitGfx(core, uploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = output.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;				

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		VulkanCommands::ImmidiateSubmitTrsf(core, uploadContext, [=](VkCommandBuffer cmd)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferImageHeight = 0;
				region.bufferRowLength = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

				vkCmdCopyBufferToImage(cmd, stagingBuffer.Buffer, output.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			});
		VulkanCommands::ImmidiateSubmitGfx(core, uploadContext, [=](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = output.Image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage;
				VkPipelineStageFlags dstStage;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			});
		
		vmaDestroyBuffer(core.Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

		return output;
	}
}
