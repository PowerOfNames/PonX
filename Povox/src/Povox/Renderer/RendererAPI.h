#pragma once
#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Pipeline.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/RenderPass.h"

#include <glm/glm.hpp>
#include <iostream>


namespace Povox {

	class RendererAPI
	{
	public:
		enum class API
		{
			NONE = 0, Vulkan = 1
		};

	public:
		virtual ~RendererAPI() = default;


		virtual void BeginFrame() = 0;
		virtual void Draw(const std::vector<Renderable>& drawList) = 0;
		virtual void EndFrame() = 0;

		virtual void UpdateCamera(Ref<Buffer> cameraUniformBuffer) = 0;

		virtual void Submit(const Renderable& object) = 0;

		virtual uint32_t GetCurrentFrameIndex() const = 0;


		virtual const void* GetCommandBuffer(uint32_t index) = 0;
		virtual void BeginCommandBuffer(const void* cmd) = 0;
		virtual void EndCommandBuffer() = 0;

		virtual void BeginRenderPass(Ref<RenderPass> renderPass) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BindPipeline(Ref<Pipeline> pipeline) = 0;


		virtual void AddImGuiImage(float width, float height) = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void OnResize(uint32_t width, uint32_t height) = 0;

		inline static void SetAPI(API api) { s_API = api; std::cout << "Changed API to " << (int)s_API << std::endl; }
		inline static RendererAPI::API GetAPI() { return s_API; }

	private:
		static API s_API;
	};

}
