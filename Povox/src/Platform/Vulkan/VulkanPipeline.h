#pragma once
#include "Povox/Renderer/Pipeline.h"

#include "Platform/Vulkan/VulkanSwapchain.h"


#include <vulkan/vulkan.h>
namespace Povox {

	class VulkanPipeline : public Pipeline
	{
	public:
		VulkanPipeline(const PipelineSpecification& specs);
		virtual ~VulkanPipeline();
		virtual void Free() override;

		virtual void Recreate(bool forceRecreate = false) override;

		inline VkPipeline GetVulkanObj() { return m_Pipeline; }
		inline VkPipelineLayout GetLayout() { return m_Layout; }
		virtual inline  PipelineSpecification& GetSpecification() override { return m_Specification; }
		virtual inline const std::unordered_map<std::string, Ref<ShaderResourceDescription>>& GetResourceDescriptions() const override;
		virtual inline const std::string& GetDebugName() const override { return m_Specification.DebugName; }

		virtual void PrintShaderLayout() const override;

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


	class VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline(const ComputePipelineSpecification& specs);
		virtual ~VulkanComputePipeline();
		virtual void Free() override;

		virtual void Recreate(bool forceRecreate = false) override;

		inline VkPipeline GetVulkanObj() { return m_Pipeline; }
		inline VkPipelineLayout GetLayout() { return m_Layout; }

		virtual inline const std::unordered_map<std::string, Ref<ShaderResourceDescription>>& GetResourceDescriptions() const override;
		virtual inline ComputePipelineSpecification& GetSpecification() { return m_Specification; }
		virtual inline const std::string& GetDebugName() const { return m_Specification.DebugName; }

		virtual void PrintShaderLayout() const override;

	private:
		void CreateLayout();
		void CreatePipeline();
		

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
		ComputePipelineSpecification m_Specification{};

		std::unordered_map<std::string, std::pair<VkDescriptorSetLayout, VkDescriptorSet>> m_DescriptorSets;

		VkPipelineShaderStageCreateInfo m_ShaderStageInfo;
	};

}
