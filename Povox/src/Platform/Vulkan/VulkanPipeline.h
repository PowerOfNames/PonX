#pragma once
#include "Povox/Renderer/Pipeline.h"

#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanShader.h"


#include <vulkan/vulkan.h>
namespace Povox {
	

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecification& specs);
		virtual ~VulkanPipeline();

		virtual void Recreate() override;
		
		inline VkPipelineLayout GetLayout() { return m_Layout; }
		inline VkPipeline GetVulkanObj() { return m_Pipeline; }
		virtual inline  PipelineSpecification& GetSpecification() override { return m_Specification; }

	private:
		void CreateLayout();
		void CreatePipeline();

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
		PipelineSpecification m_Specification{};

		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageInfos;
		VkPipelineVertexInputStateCreateInfo m_VertexInputStateInfo;
		VkPipelineInputAssemblyStateCreateInfo m_AssemblyStateInfo;
		VkViewport m_Viewport;
		VkRect2D m_Scissor;
		VkPipelineViewportStateCreateInfo m_ViewportStateInfo;
		VkPipelineRasterizationStateCreateInfo m_RasterizationStateInfo;
		VkPipelineMultisampleStateCreateInfo m_MultisampleStateInfo;
		VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateInfo;
		VkPipelineColorBlendStateCreateInfo m_ColorBlendStateInfo;
		std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStateInfos;
	};

}
