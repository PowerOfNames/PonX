#pragma once

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/OrthographicCamera.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/RendererAPI.h"
#include "Povox/Renderer/RenderPass.h"
#include "Povox/Renderer/Shader.h"



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

	class Renderer
	{
	public:
		static void Init(const RendererSpecification& specs);
		static void Shutdown();

		static bool BeginFrame();
		static void DrawRenderable(const Renderable& renderable);
		static void Draw();
		static void DrawGUI();
		static void EndFrame();

		static uint32_t GetCurrentFrameIndex();

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

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		inline static Ref<ShaderLibrary>& GetShaderLibrary() { return s_Data.ShaderLibrary; }

		static void CreateAPI(const RendererSpecification& specs);

	private:
		static Scope<RendererAPI> s_RendererAPI;
		static RendererData s_Data;
	};
}
