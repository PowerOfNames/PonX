#include "pxpch.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"

#include "Povox/Renderer/Renderable.h"

#include "glm/glm.hpp"

namespace Povox {

	namespace VulkanUtils {

		static VkPrimitiveTopology GetVulkanPrimitiveTopology(PipelineUtils::PrimitiveTopology topo)
		{
			switch (topo)
			{
				case PipelineUtils::PrimitiveTopology::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case PipelineUtils::PrimitiveTopology::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

				default: PX_CORE_ASSERT(true, "Topology false or missing!");
			}
		}

		static VkPolygonMode GetVulkanPolygonMode(PipelineUtils::PolygonMode mode)
		{
			switch (mode)
			{
				case PipelineUtils::PolygonMode::FILL: return VK_POLYGON_MODE_FILL;
				case PipelineUtils::PolygonMode::LINE: return VK_POLYGON_MODE_LINE;
				case PipelineUtils::PolygonMode::POINT: return VK_POLYGON_MODE_POINT;

				default: PX_CORE_ASSERT(true, "PolygonMode false or missing!");
			}
		}

		static VkFrontFace GetVulkanFrontFace(PipelineUtils::FrontFace face)
		{
			switch (face)
			{
				case PipelineUtils::FrontFace::COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
				case PipelineUtils::FrontFace::CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;

				default: PX_CORE_ASSERT(true, "FrontFace false!");
			}
		}

		static VkCullModeFlagBits GetVulkanCullMode(PipelineUtils::CullMode culling)
		{
			switch (culling)
			{
				case Povox::PipelineUtils::CullMode::NONE: return VK_CULL_MODE_NONE;
				case Povox::PipelineUtils::CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
				case Povox::PipelineUtils::CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
				case Povox::PipelineUtils::CullMode::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;

				default: PX_CORE_ASSERT(true, "CullMode false!");
			}
		}
	}



	struct VertexInputDescription
	{
		std::vector<VkVertexInputBindingDescription> Bindings;
		std::vector<VkVertexInputAttributeDescription> Attributes;

		VkPipelineVertexInputStateCreateFlags Flags = 0;
	};

	struct VulkanVertexData
	{
		VertexData VertexProperties;

		static VertexInputDescription GetVertexDescription()
		{
			VertexInputDescription output;

			VkVertexInputBindingDescription vertexBindingDescription{};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = sizeof(VulkanVertexData);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			output.Bindings.push_back(vertexBindingDescription);

			VkVertexInputAttributeDescription positionAttribute{};
			positionAttribute.binding = 0;
			positionAttribute.location = 0;
			positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			positionAttribute.offset = offsetof(VulkanVertexData, VertexProperties.Position);

			VkVertexInputAttributeDescription colorAttribute{};
			colorAttribute.binding = 0;
			colorAttribute.location = 1;
			colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttribute.offset = offsetof(VulkanVertexData, VertexProperties.Color);

			VkVertexInputAttributeDescription uvAttributes{};
			uvAttributes.binding = 0;
			uvAttributes.location = 2;
			uvAttributes.format = VK_FORMAT_R32G32_SFLOAT;
			uvAttributes.offset = offsetof(VulkanVertexData, VertexProperties.TexCoord);

			output.Attributes.push_back(positionAttribute);
			output.Attributes.push_back(colorAttribute);
			output.Attributes.push_back(uvAttributes);

			return output;
		}
	};

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& specs)
		:m_Specs(specs)
	{
		PX_PROFILE_FUNCTION();


		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(specs.Shader);
		//descriptor set layout: comes either from shader.reflect() OR needs to be set manually

		//std::vector<VkDescriptorSetLayout> layouts{ 2, m_GlobalDescriptorSetLayout, m_ObjectDescriptorSetLayout };
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;

		layoutInfo.setLayoutCount = 0;
		layoutInfo.pSetLayouts = nullptr;

		layoutInfo.pushConstantRangeCount = 0;		// optional
		layoutInfo.pPushConstantRanges = nullptr;	// optional
		//layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		//layoutInfo.pSetLayouts = layouts.data();

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(PushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;
		PX_CORE_VK_ASSERT(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_Layout), VK_SUCCESS, "Failed to create GraphicsPipelineLayout!");

		{
			VkPipelineVertexInputStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.vertexBindingDescriptionCount = 0;
			info.vertexAttributeDescriptionCount = 0;
			m_VertexInputStateInfo = info;

			//VertexInputDescription comes from shader-reflect
			VertexInputDescription description = VulkanVertexData::GetVertexDescription();
			m_VertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(description.Bindings.size());
			m_VertexInputStateInfo.pVertexBindingDescriptions = description.Bindings.data();
			m_VertexInputStateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(description.Attributes.size());
			m_VertexInputStateInfo.pVertexAttributeDescriptions = description.Attributes.data();
		}
		//TODO: Check what modules exist and iterate over them
		VkShaderModule vertexShaderModule = shader->GetModule(VK_SHADER_STAGE_VERTEX_BIT);
 		{
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;

			info.stage = VK_SHADER_STAGE_VERTEX_BIT;
			info.module = vertexShaderModule;
			info.pName = "main";
			m_ShaderStageInfos.push_back(info);
		}
		VkShaderModule fragmentShaderModule = shader->GetModule(VK_SHADER_STAGE_FRAGMENT_BIT);
		{
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;

			info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			info.module = fragmentShaderModule;
			info.pName = "main";
			m_ShaderStageInfos.push_back(info);
		}
		{
			VkPipelineInputAssemblyStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.topology = VulkanUtils::GetVulkanPrimitiveTopology(specs.Primitive);
			info.primitiveRestartEnable = VK_FALSE;
			m_AssemblyStateInfo = info;
		}
		{
			m_Viewport.x = 0.0f;
			m_Viewport.y = 0.0f;
			m_Viewport.width = (float)specs.TargetRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Width;
			m_Viewport.height = (float)specs.TargetRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Height;
			m_Viewport.minDepth = 0.0f;
			m_Viewport.maxDepth = 1.0f;
		}
		{
			VkPipelineRasterizationStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthClampEnable = VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
			info.rasterizerDiscardEnable = VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
			info.polygonMode = VulkanUtils::GetVulkanPolygonMode(specs.FillMode);					// determines if filled, lines or point shall be rendered
			info.lineWidth = 1.0f;									// thicker then 1.0f reuires wideLine GPU feature
			info.cullMode = VulkanUtils::GetVulkanCullMode(specs.Culling);
			info.frontFace = VulkanUtils::GetVulkanFrontFace(specs.Front);
			info.depthBiasEnable = VK_FALSE;
			m_RasterizationStateInfo = info;
		}
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
			m_MultisampleStateInfo = info;
		}
		{
			VkPipelineDepthStencilStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthTestEnable = specs.DepthTesting;
			info.depthWriteEnable = specs.DepthWriting;
			info.depthCompareOp = VK_COMPARE_OP_LESS;
			info.depthBoundsTestEnable = VK_FALSE;
			info.minDepthBounds = 0.0f;		// optional
			info.maxDepthBounds = 1.0f;		// optional
			info.stencilTestEnable = VK_FALSE;
			info.front = {};
			info.back = {};
			m_DepthStencilStateInfo = info;
		}
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
			m_ColorBlendAttachmentStateInfo = info;

			/* in pseudo code, this happens while blending
			if (blendEnable)
				finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
				finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
			else
				finalColor = newColor;
			finalColor = finalColor & colorWriteMask;
			*/
		}
		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
		{
			VkPipelineViewportStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.viewportCount = 1;
			info.pViewports = &m_Viewport;
			info.scissorCount = 1;
			info.pScissors = &m_Scissor;
			m_ViewportStateInfo = info;
		}

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
		colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateInfo.attachmentCount = 1;
		colorBlendStateInfo.pAttachments = &m_ColorBlendAttachmentStateInfo;
		colorBlendStateInfo.blendConstants[0] = 0.0f;
		colorBlendStateInfo.blendConstants[1] = 0.0f;
		colorBlendStateInfo.blendConstants[2] = 0.0f;
		colorBlendStateInfo.blendConstants[3] = 0.0f;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		// Dynamic state  -> allows to modify some of the states above during runtime without recreating the pipeline, such as blend constants, line width and viewport
		std::vector<VkDynamicState> dynamicStates =
		{			
			VK_DYNAMIC_STATE_LINE_WIDTH			
		};
		if (m_Specs.DynamicViewAndScissors)
		{
			dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
			pipelineInfo.pViewportState = nullptr;
		}
		else
		{
			pipelineInfo.pViewportState = &m_ViewportStateInfo;
		}

		VkPipelineDynamicStateCreateInfo dynamicStateInfo{}; // with no dynamic state, just pass in a nullptr later on
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();


		// The Pipeline
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(m_ShaderStageInfos.size());
		pipelineInfo.pStages = m_ShaderStageInfos.data();
		pipelineInfo.pVertexInputState = &m_VertexInputStateInfo;
		pipelineInfo.pInputAssemblyState = &m_AssemblyStateInfo;
		pipelineInfo.pRasterizationState = &m_RasterizationStateInfo;
		pipelineInfo.pMultisampleState = &m_MultisampleStateInfo;
		pipelineInfo.pDepthStencilState = &m_DepthStencilStateInfo;
		pipelineInfo.pColorBlendState = &colorBlendStateInfo;
		if (specs.DynamicViewAndScissors)
			pipelineInfo.pDynamicState = &dynamicStateInfo;
		else
			pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = std::dynamic_pointer_cast<VulkanRenderPass>(m_Specs.TargetRenderPass)->GetVulkanObj();
		pipelineInfo.subpass = 0; // index

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT is specified under flags in VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineIndex = -1;


		PX_CORE_VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), VK_SUCCESS, "Failed to create Graphics pipeline!");
	}

	VulkanPipeline::~VulkanPipeline()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		if (m_Layout)
		{
			vkDestroyPipelineLayout(device, m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
		if (m_Pipeline)
		{
			vkDestroyPipeline(device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	

}
