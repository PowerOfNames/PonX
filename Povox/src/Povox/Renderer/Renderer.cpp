#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Renderer2D.h"
#include "Platform/Vulkan/VulkanRenderer.h"

namespace Povox {

	

	Scope<RendererAPI> Renderer::s_RendererAPI = nullptr;
	RendererData Renderer::s_Data{};

	void Renderer::Init(const RendererSpecification& specs)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Renderer::Init: Starting initialization...");
		PX_CORE_INFO("Creating ShaderLibrary...");
		
		s_Data.ShaderLibrary = CreateRef<ShaderLibrary>();
		s_Data.ShaderLibrary->Add("TextureShader", Shader::Create("assets/shaders/Texture.glsl"));
		s_Data.ShaderLibrary->Add("FlatColorShader", Shader::Create("assets/shaders/FlatColor.glsl"));
		
		PX_CORE_INFO("Completed ShaderLibrary creation.");
		PX_CORE_INFO("Creating RendererBackend...");
		
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE:
			{	
				PX_CORE_ERROR("RendererAPI:None -> No RendererAPI selection!");
				PX_CORE_ASSERT(true, "RendererAPI has not been set yes!");
			}
			case RendererAPI::API::Vulkan:
			{
				PX_CORE_INFO("RendererBackend creation: Selected API: Vulkan -> Creating VulkanRenderer.");
				s_RendererAPI = CreateScope<VulkanRenderer>(specs);
			}
		}
		
		PX_CORE_INFO("Completed RendererBackend creation.");
		PX_CORE_INFO("Creating TextureSystem...");

		//TextureSystem Init depends on UploadContext, which is created during CommandControl init during VulkanRenderer::Init()
		s_Data.TextureSystem = CreateRef<TextureSystem>();

		PX_CORE_INFO("Completed TextureSystem creation.");
		PX_CORE_INFO("Initializing SubRenderers...");

		// Initialize the subrenderers (2D, Voxel, PixelSimulation, RayCasting, Scene etc.)
		Renderer2D::Init();


		PX_CORE_INFO("Completed SubRenderer initializations.");
		PX_CORE_INFO("Renderer::Init: Completed initialization.");
	}

	void Renderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Renderer::Shutdown: Starting...");
		PX_CORE_INFO("Starting SubRenderers shutdown...");

		Renderer2D::Shutdown();

		PX_CORE_INFO("Completed SubRenderers shutdown.");
		PX_CORE_INFO("Starting TextureSystem shutdown...");

		s_Data.TextureSystem->Shutdown();

		PX_CORE_INFO("Completed TextureSystem shutdown.");
		PX_CORE_INFO("Starting RendererBackend shutdown...");

		s_RendererAPI->Shutdown();

		PX_CORE_INFO("Completed RendererBackend shutdown.");
		PX_CORE_INFO("Starting ShaderLibrary shutdown...");

		s_Data.ShaderLibrary->Shutdown();

		PX_CORE_INFO("Completed ShaderLibrary shutdown.");
		PX_CORE_INFO("Renderer::Shutdown: Completed.");
	}

	bool Renderer::BeginFrame()
	{
		PX_PROFILE_FUNCTION();


		return s_RendererAPI->BeginFrame();
	}

	void Renderer::DrawRenderable(const Renderable& renderable)
	{
		s_RendererAPI->DrawRenderable(renderable);
	}
	void Renderer::Draw()
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->Draw();
	}
	void Renderer::DrawGUI()
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->DrawGUI();
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

	void Renderer::CreateFinalImage(Ref<Image2D> finalImage)
	{
		s_RendererAPI->CreateFinalImage(finalImage);
	}

	Ref<Image2D> Renderer::GetFinalImage()
	{
		return s_RendererAPI->GetFinalImage();
	}

	const void* Renderer::GetCommandBuffer(uint32_t index)
	{
		return s_RendererAPI->GetCommandBuffer(index);
	}
	const void* Renderer::GetGUICommandBuffer(uint32_t index)
	{
		return s_RendererAPI->GetGUICommandBuffer(index);
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

	void Renderer::BeginGUIRenderPass()
	{
		s_RendererAPI->BeginGUIRenderPass();
	}
	void Renderer::EndGUIRenderPass()
	{
		s_RendererAPI->EndGUIRenderPass();
	}

	void Renderer::BindPipeline(Ref<Pipeline> pipeline)
	{
		s_RendererAPI->BindPipeline(pipeline);
	}

	void Renderer::UpdateCamera(const CameraUniform& cam)
	{
		s_RendererAPI->UpdateCamera(cam);
	}

	void Renderer::Submit(const Renderable& object)
	{
		PX_PROFILE_FUNCTION();


		s_RendererAPI->Submit(object);
	}

	void Renderer::PrepareSwapchainImage(Ref<Image2D> finalImage)
	{
		s_RendererAPI->PrepareSwapchainImage(finalImage);
	}

	void* Renderer::GetGUIDescriptorSet(Ref<Image2D> image)
	{
		return s_RendererAPI->GetGUIDescriptorSet(image);
	}

	void Renderer::FramebufferResized(uint32_t width, uint32_t height)
	{
		s_RendererAPI->FramebufferResized(0, 0, width, height);
	}

	

	void Renderer::CreateAPI(const RendererSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan : s_RendererAPI = CreateScope<VulkanRenderer>(specs);
		}
	}

}
