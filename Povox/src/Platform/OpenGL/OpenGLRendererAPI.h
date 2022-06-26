#pragma once
#include "Povox/Renderer/RendererAPI.h"

namespace Povox {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		OpenGLRendererAPI() = default;
		~OpenGLRendererAPI() = default;

		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void SetClearColor(const glm::vec4& clearColor);
		void Clear();

		virtual void AddImGuiImage(float width, float height) {}
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
	};

}
