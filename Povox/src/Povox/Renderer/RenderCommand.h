#pragma once

#include "Povox/Renderer/RendererAPI.h"

namespace Povox {

	class RenderCommand
	{
	public:
		inline static void SetClearColor(glm::vec4 clearColor) { s_RendererAPI->SetClearColor(clearColor); }
		inline static void Clear() { s_RendererAPI->Clear(); }

		inline static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) { s_RendererAPI->DrawIndexed(vertexArray); }

	private:
		static RendererAPI* s_RendererAPI;
	};
}
