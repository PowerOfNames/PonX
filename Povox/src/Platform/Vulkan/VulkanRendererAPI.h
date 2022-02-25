#pragma once
#include "Povox/Renderer/RendererAPI.h"

#include "VulkanContext.h"

#include "Povox/Core/Core.h"

namespace Povox {

	class VulkanRendererAPI : public RendererAPI
	{
	public:
		VulkanRendererAPI() = default;
		~VulkanRendererAPI();

		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& clearColor) override;
		virtual void Clear() override;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;

		static void SetContext(VulkanContext* context) { m_Context = context; }
		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();
		virtual void AddImGuiImage(float width, float height) override;

	private:
		static VulkanContext* m_Context;
	};

}
