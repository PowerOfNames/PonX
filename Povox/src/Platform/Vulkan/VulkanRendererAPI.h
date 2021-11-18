#pragma once
#include "Povox/Renderer/RendererAPI.h"

#include "VulkanContext.h"

#include "Povox/Core/Core.h"

namespace Povox {

	class VulkanRendererAPI : public RendererAPI
	{
	public:
		VulkanRendererAPI();
		~VulkanRendererAPI();

		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& clearColor) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;

		static void SetContext(Ref<VulkanContext> context) { m_Context = context; }
		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();

	private:
		static Ref<VulkanContext> m_Context;
	};

}
