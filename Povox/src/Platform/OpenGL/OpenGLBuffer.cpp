#include "pxpch.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

#include <glad/glad.h>

namespace Povox {

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Vertex buffer //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size) //This constructor gets called when demanding a batch rendering
	{
		PX_PROFILE_FUNCTION();

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

	OpenGLVertexBuffer::OpenGLVertexBuffer(Vertex* vertices, uint32_t size)
	{
		PX_PROFILE_FUNCTION();


		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_DYNAMIC_DRAW);
	}
	
	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		PX_PROFILE_FUNCTION();


		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLVertexBuffer::Submit(Vertex* vertices) const
	{
		PX_PROFILE_FUNCTION();

		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	}

	void OpenGLVertexBuffer::Submit(const std::vector<Vertex*>& vertices) const
	{
		PX_PROFILE_FUNCTION();


		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
		//glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
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


		glCreateBuffers(1, &m_RendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
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