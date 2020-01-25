#pragma once

#include <glm/glm.hpp>

#include "Povox/Renderer/VertexArray.h"

namespace Povox {

	class RendererAPI
	{
	public:
		enum class API
		{
			NONE = 0, OpenGL = 1
		};

	public:
		virtual void SetClearColor(glm::vec4 clearColor) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) = 0;

		inline static API GetAPI() { return s_API; }

	private:
		static API s_API;
	};

}