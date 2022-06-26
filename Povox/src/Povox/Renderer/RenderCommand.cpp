#include "pxpch.h"
#include "Povox/Renderer/RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/Vulkan/VulkanRenderer.h"

namespace Povox {

	RendererAPI* RenderCommand::s_RendererAPI;

	void RenderCommand::Init()
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::Vulkan : s_RendererAPI = new VulkanRenderer; break;
			case RendererAPI::API::NONE : PX_CORE_ASSERT(false, "Api 'None' not supported by Povox");
		}
	}
}
