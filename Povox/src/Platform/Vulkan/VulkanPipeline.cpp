#include "pxpch.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanDebug.h"

#include "glm/glm.hpp"

namespace Povox {

//------------------RenderPass----------------------
	VkRenderPass VulkanRenderPass::CreateColorAndDepth(VulkanCoreObjects& core, const std::vector<VulkanRenderPass::Attachment>& attachments, VkSubpassDependency dependency)
	{
		std::vector<VkAttachmentReference> colorReferences;
		VkAttachmentReference depthAttachmentRef{};
		for (auto& a : attachments)
		{
			if (a.Reference.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				depthAttachmentRef = a.Reference;
			}
			else if (a.Reference.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				colorReferences.push_back(a.Reference);
			}
		}

		// Subpasses  -> usefull for example to create a sequence of post processing effects, always one subpass needed
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = colorReferences.size();
		subpass.pColorAttachments = colorReferences.data();
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::vector<VkAttachmentDescription> attachmentDescs;
		attachmentDescs.resize(attachments.size());
		for (size_t i = 0; i < attachments.size(); i++)
		{
			attachmentDescs[i] = attachments[i].Description;
		}

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachmentDescs.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkRenderPass renderPass;
		PX_CORE_VK_ASSERT(vkCreateRenderPass(core.Device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS, "Failed to create render pass!");
		return renderPass;
	}

	VkRenderPass VulkanRenderPass::CreateColor(VulkanCoreObjects& core, const std::vector<VulkanRenderPass::Attachment>& attachments, VkSubpassDependency dependency)
	{
		std::vector<VkAttachmentReference> colorReferences;
		for (auto& a : attachments)
		{
			if (a.Reference.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				colorReferences.push_back(a.Reference);
			}
			else
			{
				PX_CORE_WARN("Wrong attachment layout!");
			}
		}

		// Subpasses  -> usefull for example to create a sequence of post processing effects, always one subpass needed
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = colorReferences.size();
		subpass.pColorAttachments = colorReferences.data();

		std::vector<VkAttachmentDescription> attachmentDescs;
		attachmentDescs.resize(attachments.size());
		for (size_t i = 0; i < attachments.size(); i++)
		{
			attachmentDescs[i] = attachments[i].Description;
		}

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachmentDescs.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkRenderPass renderPass;
		PX_CORE_VK_ASSERT(vkCreateRenderPass(core.Device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS, "Failed to create render pass!");
		return renderPass;
	}

	

//------------------RenderPipeline----------------------
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
