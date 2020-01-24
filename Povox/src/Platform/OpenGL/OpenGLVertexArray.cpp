#include "pxpch.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

#include <glad/glad.h>

namespace Povox {

	static GLenum ShaderDataTypeToGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
		case Povox::ShaderDataType::Float:	return GL_FLOAT;
		case Povox::ShaderDataType::Float2:	return GL_FLOAT;
		case Povox::ShaderDataType::Float3:	return GL_FLOAT;
		case Povox::ShaderDataType::Float4:	return GL_FLOAT;
		case Povox::ShaderDataType::Mat3:	return GL_FLOAT;
		case Povox::ShaderDataType::Mat4:	return GL_FLOAT;
		case Povox::ShaderDataType::Int:	return GL_INT;
		case Povox::ShaderDataType::Int2:	return GL_INT;
		case Povox::ShaderDataType::Int3:	return GL_INT;
		case Povox::ShaderDataType::Int4:	return GL_INT;
		case Povox::ShaderDataType::Bool:	return GL_BOOL;
		}

		PX_CORE_ASSERT(false, "No such ShaderDataType defined!");
		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray()
	{
		glCreateVertexArrays(1, &m_RendererID);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_RendererID);
	}

	void OpenGLVertexArray::Bind() const
	{
		glBindVertexArray(m_RendererID);
	}

	void OpenGLVertexArray::Unbind() const
	{
		glBindVertexArray(0);
	}

	void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
	{
		PX_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Bound vertex buffer has no set layout!");
		glBindVertexArray(m_RendererID);
		vertexBuffer->Bind();

		uint32_t index = 0;
		const auto& layout = vertexBuffer->GetLayout();
		for (const auto& element : layout)
		{
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index,
				element.GetComponentCount(),
				ShaderDataTypeToGLBaseType(element.Type),
				element.Normalized ? GL_TRUE : GL_FALSE,
				layout.GetStride(),
				(const void*)element.Offset);

			index++;
		}
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
	{
		glBindVertexArray(m_RendererID);
		indexBuffer->Bind();

		m_IndexBuffer = indexBuffer;
	}

}