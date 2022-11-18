#include "pxpch.h"
#include "VulkanInitializers.h"

#include "VulkanDebug.h"
#include "VulkanBuffer.h"

namespace Povox { namespace VulkanInits {


//Pipeline
	VkPipelineShaderStageCreateInfo CreateShaderStageInfo(VkShaderStageFlagBits stageFlag, VkShaderModule module)
	{
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.stage = stageFlag;
		info.module = module;
		info.pName = "main";

		return info;
	}

	VkPipelineVertexInputStateCreateInfo CreateVertexInputStateInfo()
	{
		// Describes Vertex data layout

		VkPipelineVertexInputStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.vertexBindingDescriptionCount = 0;
		info.vertexAttributeDescriptionCount = 0;

		return info;
	}

	VkPipelineInputAssemblyStateCreateInfo CreateAssemblyStateInfo(VkPrimitiveTopology topology)
	{
		VkPipelineInputAssemblyStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.topology = topology;
		info.primitiveRestartEnable = VK_FALSE;

		return info;
	}

	VkPipelineRasterizationStateCreateInfo CreateRasterizationStateInfo(VkPolygonMode mode, VkCullModeFlags culling)
	{
		VkPipelineRasterizationStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.depthClampEnable = VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
		info.rasterizerDiscardEnable = VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
		info.polygonMode = mode;					// determines if filled, lines or point shall be rendered
		info.lineWidth = 1.0f;									// thicker then 1.0f reuires wideLine GPU feature
		info.cullMode = culling;
		info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		info.depthBiasEnable = VK_FALSE;

		return info;
	}

	VkPipelineMultisampleStateCreateInfo CreateMultisampleStateInfo()
	{
		VkPipelineMultisampleStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.sampleShadingEnable = VK_FALSE;
		info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		info.minSampleShading = 1.0f;		//	optional
		info.pSampleMask = nullptr;	//	optional
		info.alphaToCoverageEnable = VK_FALSE;	//	optional
		info.alphaToOneEnable = VK_FALSE;	//	optional

		return info;
	}

	VkPipelineColorBlendAttachmentState CreateColorBlendAttachment()
	{
		VkPipelineColorBlendAttachmentState info{};
		info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		info.blendEnable = VK_TRUE;
		info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		info.colorBlendOp = VK_BLEND_OP_ADD;
		info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		info.alphaBlendOp = VK_BLEND_OP_ADD;

		//check http://andersriggelsen.dk/glblendfunc.php for testing and understanding more!
		return info;
	}

	VkPipelineDepthStencilStateCreateInfo CreateDepthStencilSTateInfo()
	{
		VkPipelineDepthStencilStateCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		info.pNext = nullptr;

		info.depthTestEnable = VK_TRUE;
		info.depthWriteEnable = VK_TRUE;
		info.depthCompareOp = VK_COMPARE_OP_LESS;
		info.depthBoundsTestEnable = VK_FALSE;
		info.minDepthBounds = 0.0f;		// optional
		info.maxDepthBounds = 1.0f;		// optional
		info.stencilTestEnable = VK_FALSE;
		info.front = {};
		info.back = {};

		return info;
	}

	VkPipelineLayoutCreateInfo CreatePipelineLayoutInfo()
	{
		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;

		info.setLayoutCount = 0;		// optional
		info.pSetLayouts = nullptr;	// optional

		info.pushConstantRangeCount = 0;		// optional
		info.pPushConstantRanges = nullptr;	// optional

		return info;
	}

//Images
	VkImageCreateInfo CreateImageInfo(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkExtent3D extent, VkImageLayout initialLayout)
	{
		VkImageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.extent = extent;
		info.format = format;	// may not be supported by all hardware

		info.arrayLayers = 1;
		info.mipLevels = 1;
		info.tiling = tiling;
		info.initialLayout = initialLayout;
		info.usage = usage;	// used as destination from the buffer to image, and the shader needs to sample from it
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// exclusive means it will only be used by one queue family (graphics and therefore also transfer possible)
		info.samples = VK_SAMPLE_COUNT_1_BIT;	// related to multi sampling -> in attachments
		info.flags = 0;	// optional, can be used for sparse textures, in for examples for voxel terrain, to avoid using memory with AIR

		return info;
	}

	VkImageViewCreateInfo CreateImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspects)
	{
		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = format;
		
		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		
		info.subresourceRange.aspectMask = aspects;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		return info;
	}

// Descriptor
	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stage, uint32_t binding)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;		// binding in the shader
		layoutBinding.descriptorCount = 1;		// set could be an array -> count is number of elements
		layoutBinding.stageFlags = stage;
		layoutBinding.descriptorType = type;
		layoutBinding.pImmutableSamplers = nullptr;	// optional ->  for image sampling

		return layoutBinding;
	}

	VkWriteDescriptorSet CreateDescriptorWrites(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, VkDescriptorImageInfo* imageInfo, uint32_t dstBinding)
	{
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;

		write.dstBinding = dstBinding;		// again, binding in the shader
		write.dstSet = dstSet;
		write.descriptorCount = 1;		// how many elements in the array
		write.descriptorType = type;
		write.pBufferInfo = bufferInfo;
		write.pImageInfo = imageInfo;	// optional image data

		return write;
	}
} }
