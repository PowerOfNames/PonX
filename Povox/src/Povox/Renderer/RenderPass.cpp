#include "pxpch.h"
#include "RenderPass.h"

#include "Platform/Vulkan/VulkanRenderPass.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {


	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanRenderPass>(spec);
			}			
		}
		PX_CORE_ASSERT(true, "Unknown RendererAPI");
		return nullptr;
	}


	Ref<ComputePass> ComputePass::Create(const ComputePassSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanComputePass>(spec);
			}
		}
		PX_CORE_ASSERT(true, "Unknown RendererAPI");
		return nullptr;
	}
}
