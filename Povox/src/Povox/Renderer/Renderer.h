#pragma once

#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/OrthographicCamera.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/RendererAPI.h"
#include "Povox/Renderer/RenderPass.h"



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

	struct CameraUniformBuffer
	{
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
		alignas(16) glm::mat4 ViewProjMatrix;
	};

	class Renderer
	{
	public:
		static void Init(const RendererSpecification& specs);
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		static uint32_t GetCurrentFrameIndex();

		static const void* GetCommandBuffer(uint32_t index);
		static void BeginCommandBuffer(const void* cmd);
		static void EndCommandBuffer();

		static void BeginRenderPass(Ref<RenderPass> renderPass);
		static void EndRenderPass();

		static void BindPipeline(Ref<Pipeline> pipeline);

		static void UpdateCamera(Ref<Buffer> cameraUniformBuffer);

		static void OnWindowResize(uint32_t width, uint32_t height);
		static void OnFramebufferResize(uint32_t width, uint32_t height);

		static void Submit(const Renderable& object);

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		inline static Ref<ShaderLibrary>& GetShaderLibrary() { return s_Data->ShaderLibrary; }

		static void CreateAPI(const RendererSpecification& specs);
	private:

		static Scope<RendererAPI> s_RendererAPI;
		static RendererData* s_Data;
	};
}
