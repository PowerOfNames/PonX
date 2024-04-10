#include "pxpch.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Systems/MaterialSystem.h"

namespace Povox {

	

	Scope<RendererAPI> Renderer::s_RendererAPI = nullptr;

	// Core
	bool Renderer::Init(const RendererSpecification& specs)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Renderer::Init: Starting initialization...");
		
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
		bool result = s_RendererAPI->Init();

		PX_CORE_INFO("Completed RendererBackend creation.");
		PX_CORE_INFO("Initializing SubRenderers...");

		// Initialize the subrenderers (2D, Voxel, PixelSimulation, RayCasting, Scene etc.)
		//result = result && Renderer2D::Init();


		PX_CORE_INFO("Completed SubRenderer initializations.");
		PX_CORE_INFO("Renderer::Init: Completed initialization.");
		return result;
	}

	void Renderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Renderer::Shutdown: Starting...");
		PX_CORE_INFO("Starting SubRenderers shutdown...");

		//Renderer2D::Shutdown();

		PX_CORE_INFO("Completed SubRenderers shutdown.");
		PX_CORE_INFO("Starting RendererBackend shutdown...");

		s_RendererAPI->Shutdown();

		PX_CORE_INFO("Completed RendererBackend shutdown.");
		PX_CORE_INFO("Renderer::Shutdown: Completed.");
	}

	void Renderer::WaitForDeviceFinished()
	{
		s_RendererAPI->WaitForDeviceFinished();
	}

	// Frame
	bool Renderer::BeginFrame()
	{
		PX_PROFILE_FUNCTION();
		return s_RendererAPI->BeginFrame();
	}
	void Renderer::DrawRenderable(const Renderable& renderable) { s_RendererAPI->DrawRenderable(renderable); }
	void Renderer::Draw(Ref<Buffer> vertices, Ref<Material> material, Ref<Buffer> indices, size_t indexCount, bool textureless)
	{
		PX_PROFILE_FUNCTION();
		s_RendererAPI->Draw(vertices, material, indices, indexCount, textureless);
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

	uint32_t Renderer::GetCurrentFrameIndex() { return s_RendererAPI->GetCurrentFrameIndex(); }	
	uint32_t Renderer::GetLastFrameIndex() { return s_RendererAPI->GetLastFrameIndex(); }

	void Renderer::PrepareSwapchainImage(Ref<Image2D> finalImage) { s_RendererAPI->PrepareSwapchainImage(finalImage); }
	void Renderer::CreateFinalImage(Ref<Image2D> finalImage) { s_RendererAPI->CreateFinalImage(finalImage); }

	Ref<Image2D> Renderer::GetFinalImage(uint32_t frameIndex) { return s_RendererAPI->GetFinalImage(frameIndex); }


	// Resources
	const RendererSpecification& Renderer::GetSpecification() { return s_RendererAPI->GetSpecification(); }
	void* Renderer::GetGUIDescriptorSet(Ref<Image2D> image)	{ return s_RendererAPI->GetGUIDescriptorSet(image); }
	Ref<ShaderManager> Renderer::GetShaderManager() { return s_RendererAPI->GetShaderManager(); }
	Ref<TextureSystem> Renderer::GetTextureSystem()	{ return s_RendererAPI->GetTextureSystem(); }
	Ref<MaterialSystem> Renderer::GetMaterialSystem() { return s_RendererAPI->GetMaterialSystem(); }
	Ref<ShaderResourceSystem> Renderer::GetShaderResourceSystem() { return s_RendererAPI->GetShaderResourceSystem(); }

	// State
	void Renderer::OnResize(uint32_t width, uint32_t height)
	{
		s_RendererAPI->OnResize(width, height);
	}
	void Renderer::OnViewportResize(uint32_t width, uint32_t height) { s_RendererAPI->OnViewportResize(width, height); }
	void Renderer::OnSwapchainRecreate() { s_RendererAPI->OnSwapchainRecreate(); }

	// Commands
	void Renderer::BeginCommandBuffer(const void* cmd) { s_RendererAPI->BeginCommandBuffer(cmd); }
	void Renderer::EndCommandBuffer() {	s_RendererAPI->EndCommandBuffer(); }
	const void* Renderer::GetCommandBuffer(uint32_t index) { return s_RendererAPI->GetCommandBuffer(index); }

	// Renderpass
	void Renderer::BeginRenderPass(Ref<RenderPass> renderPass) { s_RendererAPI->BeginRenderPass(renderPass); }
	void Renderer::EndRenderPass() { s_RendererAPI->EndRenderPass(); }
	
	// Pipeline
	void Renderer::BindPipeline(Ref<Pipeline> pipeline) { s_RendererAPI->BindPipeline(pipeline); }
	
	// Compute
	void Renderer::DispatchCompute(Ref<ComputePass> computePass) { s_RendererAPI->DispatchCompute(computePass); }

	// GUI
	void Renderer::BeginGUIRenderPass() { s_RendererAPI->BeginGUIRenderPass(); }
	void Renderer::EndGUIRenderPass() {	s_RendererAPI->EndGUIRenderPass(); }
	const void* Renderer::GetGUICommandBuffer(uint32_t index) {	return s_RendererAPI->GetGUICommandBuffer(index); }


	// Debugging and Statistics
	const RendererStatistics& Renderer::GetStatistics() { return s_RendererAPI->GetStatistics(); }
	void Renderer::StartTimestampQuery(const std::string& name) { s_RendererAPI->StartTimestampQuery(name); }
	void Renderer::StopTimestampQuery(const std::string& name) { s_RendererAPI->StopTimestampQuery(name); }
	

	

	void Renderer::CreateAPI(const RendererSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan : s_RendererAPI = CreateScope<VulkanRenderer>(specs);
		}
	}

}
