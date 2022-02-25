#include "pxpch.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanDebug.h"

#include "glm/glm.hpp"

namespace Povox {

	VkPipeline VulkanPipeline::Create(VkDevice logicalDevice, VkRenderPass renderPass)
	{		
		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.pNext = nullptr;

		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.pViewports = &m_Viewport;
		viewportStateInfo.scissorCount = 1;
		viewportStateInfo.pScissors = &m_Scissor;
		m_ViewportStateInfo = viewportStateInfo;

		/* in pseudo code, this happens while blending
		if (blendEnable)
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
		else
			finalColor = newColor;
		finalColor = finalColor & colorWriteMask;
		*/

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

		// Dynamic state  -> allows to modify some of the states above during runtime without recreating the pipeline, such as blend constants, line width and viewport
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo{}; // with no dynamic state, just pass in a nullptr later on
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = 2;
		dynamicStateInfo.pDynamicStates = dynamicStates;


		// The Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = m_ShaderStageInfos.size();
		pipelineInfo.pStages = m_ShaderStageInfos.data();
		pipelineInfo.pVertexInputState = &m_VertexInputStateInfo;
		pipelineInfo.pInputAssemblyState = &m_AssemblyStateInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &m_RasterizationStateInfo;
		pipelineInfo.pMultisampleState = &m_MultisampleStateInfo;
		pipelineInfo.pDepthStencilState = &m_DepthStencilStateInfo;
		pipelineInfo.pColorBlendState = &colorBlendStateInfo;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0; // index

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT is specified under flags in VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineIndex = -1;

		VkPipeline pipeline;

		PX_CORE_VK_ASSERT(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), VK_SUCCESS, "Failed to create Graphics pipeline!");

		return pipeline;
	}

}
