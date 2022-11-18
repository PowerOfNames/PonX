#pragma once
#include "Povox/Renderer/Pipeline.h"

#include "VulkanInitializers.h"
#include "VulkanSwapchain.h"
#include "VulkanShader.h"


#include <vulkan/vulkan.h>
namespace Povox {
	

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecification& specs);
		virtual ~VulkanPipeline();
		
		inline VkPipelineLayout GetLayout() { return m_Layout; }
		inline VkPipeline GetVulkanObj() { return m_Pipeline; }
		virtual inline  PipelineSpecification& GetSpecification() override { return m_Specs; }

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
		PipelineSpecification m_Specs{};

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
	};

}
