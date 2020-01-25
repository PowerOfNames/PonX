#pragma once

#include "Povox/Renderer/RenderCommand.h"
#include "Povox/Renderer/OrthographicCamera.h"

#include "Povox/Renderer/Shader.h"

#include <glm/glm.hpp>

namespace Povox {

	class Renderer
	{
	public:
		static void BeginScene(OrthographicCamera& camera); // TODO: Needs to be filled with environment (e.g. lighting), camera...
		static void EndScene();

		static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};
}
