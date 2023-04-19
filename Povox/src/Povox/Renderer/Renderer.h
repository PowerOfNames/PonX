#pragma once

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Material.h"
#include "Povox/Renderer/OrthographicCamera.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/RendererAPI.h"
#include "Povox/Renderer/RenderPass.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/TextureSystem.h"



#include <glm/glm.hpp>

namespace Povox {
	struct RendererSpecification
	{
		uint32_t MaxFramesInFlight = 1;
		size_t MaxSceneObjects = 1000;
	};

	struct RendererData
	{
		Ref<ShaderLibrary> ShaderLibrary;
		Ref<TextureSystem> TextureSystem;
	};

	struct CameraUniform
	{
		glm::mat4 ViewMatrix;
		glm::mat4 ProjectionMatrix;
		glm::mat4 ViewProjMatrix;
	};

	struct SceneUniform
	{
		glm::vec4 FogColor;
		glm::vec4 FogDistance;
		glm::vec4 AmbientColor; //.a = ambientFactor
		glm::vec4 SunlightDirection;
		glm::vec4 SunlightColor;
	};

	struct ObjectUniform
	{
		//glm::mat4 ModelMatrix;
		uint32_t TexID;
		float TilingFactor;
	};

	struct RendererStatistics
	{
		uint32_t CurrentSwapchainImageIndex = 0;
		uint32_t CurrentFrameIndex = 0;
		uint32_t LastFrameIndex = 0;
		uint64_t TotalFrames = 0;

		// PipelineStatistics
		std::vector<uint64_t> PipelineStats;
		std::vector<std::string> PipelineStatNames;

		// Timestamps
		std::unordered_map<std::string, uint64_t> TimestampResults; //per 		
	};


	class Renderer
	{
	public:
		static void Init(const RendererSpecification& specs);
		static void Shutdown();

		static bool BeginFrame();
		static void DrawRenderable(const Renderable& renderable);
		static void Draw(Ref<Buffer> vertices, Ref<Material> material, Ref<Buffer> indices, size_t indexCount);
		static void DrawGUI();
		static void EndFrame();

		static uint32_t GetCurrentFrameIndex();

		static Ref<ShaderLibrary> GetShaderLibrary();
		static Ref<TextureSystem> GetTextureSystem();

		static const RendererSpecification& GetSpecification();

		static void CreateFinalImage(Ref<Image2D> finalImage);
		static Ref<Image2D> GetFinalImage();

		static const void* GetCommandBuffer(uint32_t index);
		static const void* GetGUICommandBuffer(uint32_t index);
		static void BeginCommandBuffer(const void* cmd);
		static void EndCommandBuffer();

		static void BeginRenderPass(Ref<RenderPass> renderPass);
		static void EndRenderPass();

		static void BeginGUIRenderPass();
		static void EndGUIRenderPass();

		static void BindPipeline(Ref<Pipeline> pipeline);

		static void UpdateCamera(const CameraUniform& cam);

		static void FramebufferResized(uint32_t width, uint32_t height);

		static void Submit(const Renderable& object);

		static void PrepareSwapchainImage(Ref<Image2D> finalImage);

		static void* GetGUIDescriptorSet(Ref<Image2D> image);

		static const RendererStatistics& Renderer::GetStatistics();
		static void StartTimestampQuery(const std::string& name);
		static void StopTimestampQuery(const std::string& name);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void CreateAPI(const RendererSpecification& specs);

	private:
		static Scope<RendererAPI> s_RendererAPI;
	};
}
