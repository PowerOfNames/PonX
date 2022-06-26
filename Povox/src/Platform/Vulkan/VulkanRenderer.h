#pragma once
#include "Povox/Renderer/RendererAPI.h"
#include "VulkanContext.h"

#include "VulkanImGui.h"

#include "Povox/Core/Core.h"
#include "Povox/Core/Application.h"


namespace Povox {
	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	struct FrameData
	{
		AllocatedBuffer CamUniformBuffer;
		VkDescriptorSet GlobalDescriptorSet;

		AllocatedBuffer	ObjectBuffer;
		VkDescriptorSet ObjectDescriptorSet;
	};

	class VulkanRenderer : public RendererAPI
	{
	public:
		VulkanRenderer() = default;
		~VulkanRenderer();

		virtual void Init() override;


		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;


		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;

		static void SetContext(VulkanContext* context) { m_Context = context; }
		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();
		virtual void AddImGuiImage(float width, float height) override;


	private:
		void Shutdown();
		void OnResize(uint32_t width, uint32_t height);

	private:
		static VulkanContext* m_Context;
		UploadContext m_UploadContext;

		FrameData m_Frames[MAX_FRAMES_IN_FLIGHT];	//Swapchain
		uint32_t m_CurrentFrame = 0;


		Scope<VulkanImGui> m_ImGui;
	};

}
