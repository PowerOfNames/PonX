#include "pxpch.h"
#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

namespace Povox {

	OpenGLUniformbuffer::OpenGLUniformbuffer(uint32_t size, uint32_t binding)
	{
		glCreateBuffers(1, &m_RendererID);
		//glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	OpenGLUniformbuffer::~OpenGLUniformbuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLUniformbuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
}
