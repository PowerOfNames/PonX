#include "pxpch.h"
#include "Uniformbuffer.h"

#include "Platform/OpenGL/OpenGLUniformbuffer.h"

#include "Povox/Renderer/Renderer.h"


namespace Povox {

	Ref<Uniformbuffer> Uniformbuffer::Create(uint32_t size, uint32_t binding)
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
			return CreateRef<OpenGLUniformbuffer>(size, binding);
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
