#pragma once

#include "Povox/Renderer/RenderCommand.h"

namespace Povox {

	class Renderer
	{
	public:
		static void BeginScene(); // TODO: Needs to be filled with environment (e.g. lighting), camera...
		static void EndScene();

		static void Submit(const std::shared_ptr<VertexArray>& vertexArray);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}
