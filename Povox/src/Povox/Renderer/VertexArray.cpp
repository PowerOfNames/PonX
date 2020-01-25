#include "pxpch.h"
#include "Povox/Renderer/VertexArray.h"

#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Povox/Renderer/Renderer.h"

namespace Povox {

	VertexArray* VertexArray::Create()
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
			return new OpenGLVertexArray();
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}