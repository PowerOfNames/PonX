#pragma once
#include "Povox/Renderer/RendererAPI.h"

namespace Povox {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		OpenGLRendererAPI();
		~OpenGLRendererAPI();

		virtual void SetClearColor(glm::vec4 clearColor) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray) override;
	};

}
