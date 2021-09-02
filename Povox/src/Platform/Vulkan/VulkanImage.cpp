#include "pxpch.h"
#include "VulkanImage.h"

#include "VulkanDebug.h"
#include "VulkanUtility.h"

namespace Povox {

	void VulkanImage::Create(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkImage& image, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory& memory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.mipLevels = 1;
		imageInfo.format = format;	// may not be supported by all hardware
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;	// used as destination from the buffer to image, and the shader needs to sample from it
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// exclusive means it will only be used by one queue family (graphics and thgerefore also transfer possible)
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;	// related to multi sampling -> in attachments
		imageInfo.flags = 0;	// optional, can be used for sparse textures, in "D vfor examples for voxel terrain, to avoid using memory with AIR


		PX_CORE_VK_ASSERT(vkCreateImage(logicalDevice, &imageInfo, nullptr, &image), VK_SUCCESS, "Failed to create Texture Image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

		VkMemoryAllocateInfo memoryInfo{};
		memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryInfo.allocationSize = memRequirements.size;
		memoryInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);


		PX_CORE_VK_ASSERT(vkAllocateMemory(logicalDevice, &memoryInfo, nullptr, &memory), VK_SUCCESS, "Failed to allocate image memory!");

		vkBindImageMemory(logicalDevice, image, memory, 0);
	}



	void VulkanImage::Destroy(VkDevice logicalDevice, VkImage image)
	{
		vkDestroyImage(logicalDevice, image, nullptr);
	}

	

	void VulkanImageView::Create(VkDevice logicalDevice, VkImageView& imageView, VkImage image, VkFormat format, VkImageAspectFlags aspects)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = aspects;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;


		PX_CORE_VK_ASSERT(vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageView), VK_SUCCESS, "Failed to create image view!");
	}

	void VulkanImageView::Destroy(VkDevice logicalDevice, VkImageView imageView)
	{
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
}