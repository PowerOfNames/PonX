#pragma once

#include "Povox/Renderer/VertexArray.h"

namespace Povox {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		~OpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
		virtual void SubmitVertexData(Vertex* vertices, size_t size) const override;
		virtual void SubmitVertexData(const std::vector<Vertex*>& vertices) const override;
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

		inline virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		inline virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

	private:
		uint32_t m_RendererID;
		std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer> m_IndexBuffer;
	};

}