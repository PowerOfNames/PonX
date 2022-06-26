#pragma once

#include "Povox/Renderer/RendererAPI.h"

namespace Povox {

	class RenderCommand
	{
	public:
		static void Init();


		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) 
		{ 
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		inline static void AddImGuiImage(float width, float height)
		{
			s_RendererAPI->AddImGuiImage(width, height);
		}

	private:
		static RendererAPI* s_RendererAPI;
	};

}
