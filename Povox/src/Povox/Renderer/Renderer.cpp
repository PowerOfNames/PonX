#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Renderer2D.h"
#include "Platform/Vulkan/VulkanRenderer.h"

namespace Povox {


	RendererData* Renderer::s_Data;
	void Renderer::Init(const RendererSpecification& specs)
	{
		PX_PROFILE_FUNCTION();


		//Initialize main API
		s_RendererAPI = CreateScope<VulkanRenderer>(specs);

		// Initialize the subrenderers (2D, Voxel, PixelSimulation, RayCasting, Scene)
		Renderer2D::Init();
		
		//Shader Loading
		//s_Data->ShaderLibrary->Add("GeometryShader", Shader::Create("assets/shaders/geometryShader.glsl"));
		s_Data->ShaderLibrary->Add("StandardShader", Shader::Create("assets/shaders/standardShader.glsl"));
		//s_Data->ShaderLibrary->Add("CompositeShader", Shader::Create("assets/shaders/geometryShader.glsl"));

		//Texture Loading here or in specific Renderer
	}

	void Renderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		//clear libs, shutdown the renderers
		Renderer2D::Shutdown();


	}

	void Renderer::BeginFrame()
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->EndFrame();
	}
	
	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_RendererAPI->GetCurrentFrameIndex();
	}

	const void* Renderer::GetCommandBuffer(uint32_t index)
	{
		return s_RendererAPI->GetCommandBuffer(index);
	}

	void Renderer::BeginCommandBuffer(const void* cmd)
	{
		s_RendererAPI->BeginCommandBuffer(cmd);
	}

	void Renderer::EndCommandBuffer()
	{
		s_RendererAPI->EndCommandBuffer();
	}

	void Renderer::BeginRenderPass(Ref<RenderPass> renderPass)
	{
		s_RendererAPI->BeginRenderPass(renderPass);
	}

	void Renderer::EndRenderPass()
	{
		s_RendererAPI->EndRenderPass();
	}

	void Renderer::BindPipeline(Ref<Pipeline> pipeline)
	{
		s_RendererAPI->BindPipeline(pipeline);
	}

	void Renderer::UpdateCamera(Ref<Buffer> cameraUniformBuffer)
	{
		s_RendererAPI->UpdateCamera(cameraUniformBuffer);
	}

	void Renderer::Submit(const Renderable& object)
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->Submit(object);
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->SetViewport(0, 0, width, height);
	}

	void Renderer::OnFramebufferResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->SetViewport(0, 0, width, height);
	}

	

	void Renderer::CreateAPI(const RendererSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::Vulkan : s_RendererAPI = CreateScope<VulkanRenderer>(specs);
		}
	}

}
