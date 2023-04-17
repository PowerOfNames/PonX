#pragma once
#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Material.h"
#include "Povox/Renderer/Pipeline.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/RenderPass.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/TextureSystem.h"

#include <glm/glm.hpp>
#include <iostream>


namespace Povox {

	struct CameraUniform;
	struct RendererSpecification;
	class RendererAPI
	{
	public:
		enum class API
		{
			NONE = 0, Vulkan = 1
		};
		static const std::string APIAsString(API api)
		{
			std::string apiName;
			switch (api)
			{
			case API::NONE : return "None";
			case API::Vulkan: return "Vulkan";
			}
			PX_CORE_ASSERT(true, "No valid API selected!");
		}
	public:
		virtual ~RendererAPI() = default;

		virtual void Shutdown() = 0;

		virtual bool BeginFrame() = 0;
		//virtual void Draw(const std::vector<Renderable>& drawList) = 0;
		virtual void DrawRenderable(const Renderable& renderable) = 0;
		virtual void Draw(Ref<Buffer> vertices, Ref<Material> material, Ref<Buffer> indices, size_t indexCount) = 0;
		virtual void DrawGUI() = 0;
		virtual void EndFrame() = 0;

		virtual void UpdateCamera(const CameraUniform& cam) = 0;

		virtual Ref<ShaderLibrary> GetShaderLibrary() = 0;
		virtual Ref<TextureSystem> GetTextureSystem() = 0;

		virtual const RendererSpecification& GetSpecification() = 0;

		virtual void Submit(const Renderable& object) = 0;

		virtual uint32_t GetCurrentFrameIndex() const = 0;

		virtual void CreateFinalImage(Ref<Image2D> finalImage) = 0;
		virtual Ref<Image2D> GetFinalImage() = 0;

		virtual const void* GetCommandBuffer(uint32_t index) = 0;
		virtual const void* GetGUICommandBuffer(uint32_t index) = 0;
		virtual void BeginCommandBuffer(const void* cmd) = 0;
		virtual void EndCommandBuffer() = 0;

		virtual void BeginRenderPass(Ref<RenderPass> renderPass) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BeginGUIRenderPass() = 0;
		virtual void EndGUIRenderPass() = 0;

		virtual void BindPipeline(Ref<Pipeline> pipeline) = 0;

		virtual void PrepareSwapchainImage(Ref<Image2D> finalImage) = 0;

		virtual void FramebufferResized(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void* GetGUIDescriptorSet(Ref<Image2D> image) = 0;

		virtual void GetPipelineStats(std::vector<std::string>& names, std::vector<uint64_t>& values) = 0;
		virtual void GetQueryResults() = 0;

		inline static void SetAPI(API api) { s_API = api; std::cout << "Changed API to " << RendererAPI::APIAsString(s_API) << std::endl; }
		inline static RendererAPI::API GetAPI() { return s_API; }

	private:
		static API s_API;
	};

}
