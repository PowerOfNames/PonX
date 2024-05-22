#pragma once
#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Pipeline.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/RenderPass.h"
#include "Povox/Resources/ShaderManager.h"
#include "Povox/Systems/MaterialSystem.h"
#include "Povox/Systems/ShaderResourceSystem.h"
#include "Povox/Systems/TextureSystem.h"


#include <glm/glm.hpp>
#include <iostream>


namespace Povox {

	struct CameraUniform;
	struct RendererSpecification;
	struct RendererStatistics;
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

		// Core
		virtual bool Init() = 0;
		virtual void Shutdown() = 0;

		// Render
		virtual bool BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void DrawRenderable(const Renderable& renderable) = 0;
		virtual void Draw(Ref<Buffer> vertices, uint64_t firstVertexOffset, Ref<Material> material, Ref<Buffer> indices, size_t indexCount, bool textureless) = 0;

		virtual uint32_t GetCurrentFrameIndex() const = 0;
		virtual uint32_t GetLastFrameIndex() const = 0;

		virtual void CreateFinalImage(Ref<Image2D> finalImage) = 0;
		virtual Ref<Image2D> GetFinalImage(uint32_t frameIndex) const = 0;
		virtual void PrepareSwapchainImage(Ref<Image2D> finalImage) = 0;

		// State
		virtual void OnResize(uint32_t width, uint32_t height) = 0;
		virtual void OnViewportResize(uint32_t width, uint32_t height) = 0;
		virtual void OnSwapchainRecreate() = 0;
		virtual void WaitForDeviceFinished() = 0;



		// Resources
		virtual Ref<ShaderManager> GetShaderManager() const = 0;
		virtual Ref<TextureSystem> GetTextureSystem() const = 0;
		virtual Ref<MaterialSystem> GetMaterialSystem() const = 0;
		virtual Ref<ShaderResourceSystem> GetShaderResourceSystem() const = 0;

		virtual const RendererSpecification& GetSpecification() const = 0;
		virtual void* GetGUIDescriptorSet(Ref<Image2D> image) const = 0;

		// Commands 
		virtual const void* GetCommandBuffer(uint32_t index) = 0;
		virtual const void* GetGUICommandBuffer(uint32_t index) = 0;
		virtual void BeginCommandBuffer(const void* cmd) = 0;
		virtual void EndCommandBuffer() = 0;

		// Renderpass
		virtual void BeginRenderPass(Ref<RenderPass> renderPass) = 0;
		virtual void EndRenderPass() = 0;

		// Compute
		virtual void DispatchCompute(Ref<ComputePass> computePass, uint64_t totalElements, uint32_t workGroupWeightX, uint32_t workGroupWeightY, uint32_t workGroupWeightZ) = 0;

		// GUI
		virtual void DrawGUI() = 0;
		virtual void BeginGUIRenderPass() = 0;
		virtual void EndGUIRenderPass() = 0;

		// Pipeline
		virtual void BindPipeline(Ref<Pipeline> pipeline) = 0;


		// Debugging and Statistics
		virtual const RendererStatistics& GetStatistics() const = 0;
		virtual void StartTimestampQuery(const std::string& name) = 0;
		virtual void StopTimestampQuery(const std::string& name) = 0;
		virtual void AddTimestampQuery(const std::string& name, uint32_t count) = 0;

		virtual void AddPipelineStatisticsQuery(const std::string& name, const std::string& computeStatQueryPoolName) = 0;
		virtual const std::string& GetComputeStatisticsQueryPoolName() const = 0;


		inline static void SetAPI(API api) { s_API = api; std::cout << "Changed API to " << RendererAPI::APIAsString(s_API) << std::endl; }
		inline static RendererAPI::API GetAPI() { return s_API; }

	private:
		static API s_API;
	};

}
