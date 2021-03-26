#include "pxpch.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

#include <glad/glad.h>

#include "Povox/Renderer/QuadVertexStruct.h"


namespace Povox {

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Vertex buffer //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("VertexBuffer Size: {0}", size);

		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
	{
		PX_PROFILE_FUNCTION();


		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	void OpenGLVertexBuffer::SetData(const void* data, uint32_t size)
	{
		PX_PROFILE_FUNCTION();

		//for (uint32_t i = 0; i < 16; i++)
		//	std::cout << "QuadVertexColor: " << ((QuadVertex*)data)[i].Color.x << "|" << ((QuadVertex*)data)[i].Color.y << "|" << ((QuadVertex*)data)[i].Color.z << "|" << ((QuadVertex*)data)[i].Color.a << std::endl;


		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		PX_PROFILE_FUNCTION();


		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLVertexBuffer::Bind() const
	{
		PX_PROFILE_FUNCTION();


		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	}

	void OpenGLVertexBuffer::Unbind() const
	{
		PX_PROFILE_FUNCTION();


		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Index buffer ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
		:m_Count(count)
	{
		PX_PROFILE_FUNCTION();
		
		//for (uint32_t i = 0; i < count; i++)
		//	PX_CORE_INFO("Index: {0}", indices[i]);

		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}
	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		PX_PROFILE_FUNCTION();


		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLIndexBuffer::Bind() const
	{
		PX_PROFILE_FUNCTION();


		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
	}

	void OpenGLIndexBuffer::Unbind() const
	{
		PX_PROFILE_FUNCTION();


		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}