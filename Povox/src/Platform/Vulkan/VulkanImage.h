#pragma once
#include <vulkan/vulkan.h>

namespace Povox {

	class VulkanImage
	{
	public:
		static void Create(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkImage& image, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory& memory);
		static void Destroy(VkDevice logicalDevice, VkImage image);

	};

	class VulkanImageView
	{
	public:
		static void Create(VkDevice logicalDevice, VkImageView& imageView, VkImage image, VkFormat format, VkImageAspectFlags aspects);
		static void Destroy(VkDevice logicalDevice, VkImageView imageView);

	};
}