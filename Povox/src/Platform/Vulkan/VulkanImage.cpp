#include "pxpch.h"
#include "VulkanImage.h"

#include "VulkanDebug.h"
#include "VulkanInitializers.h"

#include "VulkanBuffer.h"
#include "VulkanCommands.h"

#include <stb_image.h>


namespace Povox {

	AllocatedImage VulkanImage::Create(VulkanCoreObjects& core, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage)
	{
		VkExtent3D extent;
		extent.width = static_cast<uint32_t>(width);
		extent.height = static_cast<uint32_t>(height);
		extent.depth = 1;

		VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(format, tiling, usage, extent);

		AllocatedImage newImage;

		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		PX_CORE_VK_ASSERT(vmaCreateImage(core.Allocator, &imageInfo, &allocationInfo, &newImage.Image, &newImage.Allocation, nullptr), VK_SUCCESS, "Failed to create Texture Image!");

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

		AllocatedImage output;
		output = VulkanImage::Create(core, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

		VulkanCommands::TransitionImageLayout(core, uploadContext, output.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VulkanCommands::CopyBufferToImage(core, uploadContext, stagingBuffer.Buffer, output.Image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));																		//-------- Command
		VulkanCommands::TransitionImageLayout(core, uploadContext, output.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vmaDestroyBuffer(core.Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

		return output;
	}
}
