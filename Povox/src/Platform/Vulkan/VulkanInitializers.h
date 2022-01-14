#pragma once


#include <vulkan/vulkan.h>

namespace Povox {	namespace VulkanInits {
//Command pool
	VkCommandPoolCreateInfo CreateCommandPoolInfo(uint32_t familyIndex, VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CreateCommandBufferInfo(VkCommandPool commandPool, uint32_t size);
	VkCommandBufferBeginInfo BeginCommandBuffer(VkCommandBufferUsageFlags flags = 0);

//Render pass
	VkRenderPassBeginInfo BeginRenderPass(VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer framebuffer);

//Pipeline creation
	VkPipelineShaderStageCreateInfo CreateShaderStageInfo(VkShaderStageFlagBits stageFlag, VkShaderModule module);
	VkPipelineVertexInputStateCreateInfo CreateVertexInputStateInfo();
	VkPipelineInputAssemblyStateCreateInfo CreateAssemblyStateInfo(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo CreateRasterizationStateInfo(VkPolygonMode mode, VkCullModeFlags culling);
	VkPipelineMultisampleStateCreateInfo CreateMultisampleStateInfo();
	VkPipelineColorBlendAttachmentState CreateColorBlendAttachment();
	VkPipelineDepthStencilStateCreateInfo CreateDepthStencilSTateInfo();
	VkPipelineLayoutCreateInfo CreatePipelineLayoutInfo();

//Images
	VkImageCreateInfo CreateImageInfo(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkExtent3D extent);
	VkImageViewCreateInfo CreateImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspects);

// Descriptors
	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t binding);
	VkWriteDescriptorSet CreateDescriptorWrites(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo, uint32_t dstBinding);

// Framebuffer
	VkFramebufferCreateInfo CreateFramebufferInfo(VkRenderPass renderPass, uint32_t width, uint32_t height);
} }
