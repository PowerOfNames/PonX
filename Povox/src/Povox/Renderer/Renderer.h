#pragma once

#include "Povox/Renderer/RenderCommand.h"
#include "Povox/Renderer/OrthographicCamera.h"

#include "Povox/Renderer/Shader.h"

#include <glm/glm.hpp>

namespace Povox {

	class Renderer
	{
	public:
		static void Init();

		static void BeginScene(OrthographicCamera& camera); // TODO: Needs to be filled with environment (e.g. lighting), camera...
		static void EndScene();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;
	};
}
