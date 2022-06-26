#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Renderer2D.h"
#include "Povox/Renderer/VoxelRenderer.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Platform/OpenGL/OpenGLShader.h"

namespace Povox {

	Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

	RendererData* Renderer::s_Data;
	void Renderer::Init()
	{
		PX_PROFILE_FUNCTION();

		//maybe get rid of the RenderCommand API. depends on if I want to include more then just Vulkan... with only vulkan, I can hardcode the vulkan renderer
		// here and use that. Also, Renderer2D should be an abstraction of the rendererAPI to take care of 2D GEOMETRY, I will create another for GUI stuff
		// (maybe move ImGui there for testing), also adding a dedicated 3DRenderer for 3D basig geometry
		// one for Voxel based Rendering, one for fancy stuff (ComputeSimulations? maybe, this could also just be a custom project, like Povosom)
		
		RenderCommand::Init(); //For now, leave it like that -> This takes care of the main rendering API commands


		//Renderer2D::Init();
		//VoxelRenderer::Init();
		

		s_Data->ShaderLibrary->Add("GeometryShader", Shader::Create("assets/shaders/geometryShader.vert"));
	}

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
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

	void Renderer::OnFramebufferResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::SetViewport(0, 0, width, height);
	}

	// TODO: later should submit every call into a render command queue
	void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		PX_PROFILE_FUNCTION();


		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		shader->SetMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

	void Renderer::CreateAPI(const RendererSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::Vulkan : s_RendererAPI = CreateScope<VulkanRenderer>(specs);
		}
	}

}
