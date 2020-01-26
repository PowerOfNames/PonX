#pragma once

#include "Povox/Renderer/RendererAPI.h"

namespace Povox {

	class RenderCommand
	{
	public:
		inline static void SetClearColor(glm::vec4 clearColor) { s_RendererAPI->SetClearColor(clearColor); }
		inline static void Clear() { s_RendererAPI->Clear(); }

		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) { s_RendererAPI->DrawIndexed(vertexArray); }

	private:
		static RendererAPI* s_RendererAPI;
	};
}
