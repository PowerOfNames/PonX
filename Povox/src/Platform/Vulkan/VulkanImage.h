#pragma once
#include "VulkanUtility.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Povox {

	struct AllocatedImage
	{
		VkImage Image;

		VmaAllocation Allocation;
	};

	class VulkanImage
	{
	public:
		static AllocatedImage LoadFromFile(VulkanCoreObjects& core, UploadContext& uploadContext, const char* path, VkFormat format);
		static AllocatedImage Create(VulkanCoreObjects& core, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage);
	};
}
