#pragma once

#include <glm/glm.hpp>

#include "Povox/Renderer/VertexArray.h"

namespace Povox {

	class RendererAPI
	{
	public:
		enum class API
		{
			NONE = 0, OpenGL = 1, Vulkan = 2
		};

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;

		virtual void SetClearColor(glm::vec4 clearColor) = 0;
		virtual void Clear() = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) = 0;

		inline static void SetAPI(API api) { s_API = api; }
		inline static API GetAPI() { return s_API; }


	private:
		static API s_API;
	};

}