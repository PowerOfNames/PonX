#include "pxpch.h"
#include "Povox/Renderer/Buffer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {

	VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::OpenGL:
			{
				return new OpenGLVertexBuffer(vertices, size);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::OpenGL:
			{
				return new OpenGLIndexBuffer(indices, size);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}