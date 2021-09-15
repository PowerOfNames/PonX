#include "pxpch.h"
#include "GraphicsContext.h"

#include "Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Platform/Vulkan/VulkanContext.h"


namespace Povox {

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE: PX_CORE_ASSERT(false, "RendererApi::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL: return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
			case RendererAPI::API::Vulkan: return CreateScope<VulkanContext>(static_cast<GLFWwindow*>(window));
		}

		PX_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


}