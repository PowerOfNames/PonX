#include "pxpch.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanDebug.h"

#include "VulkanContext.h"

#include "glm/glm.hpp"

namespace Povox {

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& specs)
		:m_Specs(specs)
	{
		PX_PROFILE_FUNCTION();


		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(specs.Shader);

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

			VertexInputDescription description = VertexData::GetVertexDescription();
			m_VertexInputStateInfo.vertexBindingDescriptionCount = description.Bindings.size();
			m_VertexInputStateInfo.pVertexBindingDescriptions = description.Bindings.data();
			m_VertexInputStateInfo.vertexAttributeDescriptionCount = description.Attributes.size();
			m_VertexInputStateInfo.pVertexAttributeDescriptions = description.Attributes.data();
		}

		VkShaderModule vertexShaderModule = VulkanShader::Create(device, "assets/shaders/defaultVS.vert");
		{
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;

			info.stage = VK_SHADER_STAGE_VERTEX_BIT;
			info.module = shader->GetModule();
			info.pName = "main";
			m_ShaderStageInfos.push_back(info);
		}
		VkShaderModule fragmentShaderModule = VulkanShader::Create(device, "assets/shaders/defaultLit.frag");
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

			info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			info.primitiveRestartEnable = VK_FALSE;
			m_AssemblyStateInfo = info;
		}
		{
			m_Viewport.x = 0.0f;
			m_Viewport.y = 0.0f;
			m_Viewport.width = (float)specs.TargetRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Width;
			m_Viewport.height = (float)m_Specs.TargetRenderPass->GetSpecification().TargetFramebuffer->GetSpecification().Height;
			m_Viewport.minDepth = 0.0f;
			m_Viewport.maxDepth = 1.0f;
		}
		{
			VkPipelineRasterizationStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthClampEnable = VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
			info.rasterizerDiscardEnable = VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
			info.polygonMode = VK_POLYGON_MODE_FILL;					// determines if filled, lines or point shall be rendered
			info.lineWidth = 1.0f;									// thicker then 1.0f reuires wideLine GPU feature
			info.cullMode = VK_CULL_MODE_NONE;
			info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

			info.depthTestEnable = VK_TRUE;
			info.depthWriteEnable = VK_TRUE;
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
		pipelineInfo.stageCount = m_ShaderStageInfos.size();
		pipelineInfo.pStages = m_ShaderStageInfos.data();
		pipelineInfo.pVertexInputState = &m_VertexInputStateInfo;
		pipelineInfo.pInputAssemblyState = &m_AssemblyStateInfo;
		pipelineInfo.pRasterizationState = &m_RasterizationStateInfo;
		pipelineInfo.pMultisampleState = &m_MultisampleStateInfo;
		pipelineInfo.pDepthStencilState = &m_DepthStencilStateInfo;
		pipelineInfo.pColorBlendState = &colorBlendStateInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;

		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = std::dynamic_pointer_cast<VulkanRenderPass>(m_Specs.TargetRenderPass)->GetVulkanRenderPass();
		pipelineInfo.subpass = 0; // index

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT is specified under flags in VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineIndex = -1;


		PX_CORE_VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), VK_SUCCESS, "Failed to create Graphics pipeline!");

	}

	VulkanPipeline::~VulkanPipeline()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		if (m_Layout)
			vkDestroyPipelineLayout(device, m_Layout, nullptr);
		if (m_Pipeline)
			vkDestroyPipeline(device, m_Pipeline, nullptr);
	}

	

}
