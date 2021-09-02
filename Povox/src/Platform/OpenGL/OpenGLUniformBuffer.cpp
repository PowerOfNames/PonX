#include "pxpch.h"
#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

namespace Povox {

	OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding)
	{
		PX_PROFILE_FUNCTION();


		glCreateBuffers(size, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer()
	{
		PX_PROFILE_FUNCTION();


		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		PX_PROFILE_FUNCTION();


		glNamedBufferSubData(m_RendererID, offset, size, data);
	}
}