#include "pxpch.h"
#include "Framebuffer.h"

#include "Povox/Core/Core.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {

	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::API::OpenGL:
			{
				return CreateRef<OpenGLFramebuffer>(spec);
			}
			case RendererAPI::API::Vulkan:
			{
				//return CreateRef<VulkanFramebuffer>(spec);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
