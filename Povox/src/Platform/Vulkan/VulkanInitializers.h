#pragma once


#include <vulkan/vulkan.h>

namespace Povox {	namespace VulkanInits {
//Images
	VkImageCreateInfo CreateImageInfo(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkExtent3D extent, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
	VkImageViewCreateInfo CreateImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspects);

// Descriptors
	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t binding);
	VkWriteDescriptorSet CreateDescriptorWrites(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo, uint32_t dstBinding);

} }
