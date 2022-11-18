#include "pxpch.h"
#include "Framebuffer.h"

#include "Platform/Vulkan/VulkanFramebuffer.h"

#include "Povox/Core/Core.h"
#include "Povox/Renderer/Renderer.h"

namespace Povox {

	Ref<Framebuffer> Framebuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::NONE:
		{
			PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
			return nullptr;
		}
		case RendererAPI::API::Vulkan:
		{
			return CreateRef<VulkanFramebuffer>();
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanFramebuffer>(spec);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
