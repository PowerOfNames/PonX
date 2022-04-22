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
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
