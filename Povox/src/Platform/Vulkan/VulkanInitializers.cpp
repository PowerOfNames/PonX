#include "pxpch.h"
#include "VulkanInitializers.h"

#include "VulkanDebug.h"
#include "VulkanBuffer.h"

namespace Povox { namespace VulkanInits {
//CommandPool
	VkCommandPoolCreateInfo CreateCommandPoolInfo(uint32_t familyIndex, VkCommandPoolCreateFlags flags)
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;

		info.queueFamilyIndex = familyIndex;
		info.flags = flags;

		return info;
	}

	VkCommandBufferAllocateInfo CreateCommandBufferInfo(VkCommandPool commandPool, uint32_t size)
	{
		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = commandPool;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		info.commandBufferCount = size;

		return info;
	}

	VkCommandBufferBeginInfo BeginCommandBuffer(VkCommandBufferUsageFlags flags)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = flags;
		beginInfo.pInheritanceInfo = nullptr;	// optional, only used if buffer is secondary

		return beginInfo;
	}

//Render pass
	VkRenderPassBeginInfo BeginRenderPass(VkRenderPass renderPass, VkExtent2D extent, VkFramebuffer framebuffer)
	{
		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = renderPass;
		info.framebuffer = framebuffer;
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent = extent;		

		return info;
	}


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
		info.blendEnable = VK_FALSE;
		info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// optional
		info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;		// optional
		info.colorBlendOp = VK_BLEND_OP_ADD;			// optional
		info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// optional
		info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;		// optional
		info.alphaBlendOp = VK_BLEND_OP_ADD;

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
	VkImageCreateInfo CreateImageInfo(VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkExtent3D extent)
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
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.usage = usage;	// used as destination from the buffer to image, and the shader needs to sample from it
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// exclusive means it will only be used by one queue family (graphics and thgerefore also transfer possible)
		info.samples = VK_SAMPLE_COUNT_1_BIT;	// related to multi sampling -> in attachments
		info.flags = 0;	// optional, can be used for sparse textures, in "D vfor examples for voxel terrain, to avoid using memory with AIR

		return info;
	}

	VkImageViewCreateInfo CreateImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlagBits aspects)
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

// Framebuffer
	VkFramebufferCreateInfo CreateFramebufferInfo(VkRenderPass renderPass, uint32_t width, uint32_t height)
	{
		VkFramebufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.pNext = nullptr;
		info.width = width;
		info.height = height;
		info.renderPass = renderPass;
		info.layers = 1;

		return info;
	}
} }
