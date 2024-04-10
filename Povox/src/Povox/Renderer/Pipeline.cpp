#include "pxpch.h"
#include "Pipeline.h"

#include "Povox/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanPipeline.h"


namespace Povox {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				Ref<VulkanPipeline> pipeline = CreateRef<VulkanPipeline>(specs);
				specs.Shader->AddPipeline(pipeline);
				return pipeline;
			}
		}
		PX_CORE_ASSERT(true, "Renderer API not supported!");
	}

	Ref<ComputePipeline> ComputePipeline::Create(const ComputePipelineSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				Ref<VulkanComputePipeline> pipeline = CreateRef<VulkanComputePipeline>(specs);
				specs.Shader->AddComputePipeline(pipeline);
				return pipeline;
			}
		}
		PX_CORE_ASSERT(true, "Renderer API not supported!");
	}
}

