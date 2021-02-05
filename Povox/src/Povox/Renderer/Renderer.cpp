#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Renderer2D.h"
#include "Povox/Renderer/VoxelRenderer.h"

#include "Platform/OpenGL/OpenGLShader.h"

namespace Povox {

	Renderer::SceneData* Renderer::m_SceneData = new Renderer::SceneData();

	void Renderer::Init()
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::Init();
		Renderer2D::Init();
		//VoxelRenderer::Init();
	}

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		m_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
		PX_PROFILE_FUNCTION();


	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::SetViewport(0, 0, width, height);
	}

	// TODO: later should submit every call into a render command queue
	void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		PX_PROFILE_FUNCTION();


		shader->Bind();
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

}