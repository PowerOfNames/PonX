#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"

namespace Povox {

	Renderer::SceneData* Renderer::m_SceneData = new Renderer::SceneData();

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		m_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	// TODO: later should submit every call into a render command queue
	void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		shader->Bind();
		shader->SetUniformMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
		shader->SetUniformMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

}