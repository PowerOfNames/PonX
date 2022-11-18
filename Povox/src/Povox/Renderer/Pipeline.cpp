#include "pxpch.h"
#include "Pipeline.h"

#include "Povox/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanPipeline.h"


namespace Povox {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan: return CreateRef<VulkanPipeline>(specs);
		}
		PX_CORE_ASSERT(true, "Renderer API not supported!");
	}
}

