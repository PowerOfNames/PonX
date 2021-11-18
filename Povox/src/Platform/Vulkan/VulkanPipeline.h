#pragma once
#include <vulkan/vulkan.h>

#include "VulkanSwapchain.h"
#include "VulkanShader.h"

#include "VulkanInitializers.h"

namespace Povox {

	

	class VulkanRenderPass
	{
	public:
		struct Attachment
		{
			VkAttachmentDescription Description{};
			VkAttachmentReference Reference{};
		};

		static VkRenderPass CreateColorAndDepth(VulkanCoreObjects& core, const std::vector<VulkanRenderPass::Attachment>& attachments, VkSubpassDependency dependency);
		static VkRenderPass CreateColor(VulkanCoreObjects& core, const std::vector<VulkanRenderPass::Attachment>& attachments, VkSubpassDependency dependency);
	};

	class VulkanPipeline
	{
	public:
		VkPipeline Create(VkDevice logicalDevice, VkRenderPass renderPass);
	public:
		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageInfos;
		VkPipelineVertexInputStateCreateInfo m_VertexInputStateInfo;
		VkPipelineInputAssemblyStateCreateInfo m_AssemblyStateInfo;
		VkViewport m_Viewport;
		VkRect2D m_Scissor;
		VkPipelineViewportStateCreateInfo m_ViewportStateInfo;
		VkPipelineRasterizationStateCreateInfo m_RasterizationStateInfo;
		VkPipelineMultisampleStateCreateInfo m_MultisampleStateInfo;
		VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateInfo;
		VkPipelineColorBlendAttachmentState m_ColorBlendAttachmentStateInfo;
		VkPipelineLayout m_PipelineLayout;
	};

}
