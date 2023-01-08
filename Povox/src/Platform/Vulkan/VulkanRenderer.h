#pragma once
#include "Platform/Vulkan/VulkanCommands.h"

#include "Platform/Vulkan/VulkanImGui.h"

#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanRenderPass.h"

#include "Povox/Core/Application.h"
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/RendererAPI.h"


namespace Povox {

	struct FrameData
	{
		struct FrameSemaphores
		{
			VkSemaphore RenderSemaphore;
			VkSemaphore PresentSemaphore;
		};
		FrameSemaphores Semaphores;
		VkFence Fence;

		struct FrameCommandBuffer
		{
			VkCommandBuffer Buffer;
			VkCommandPool Pool;
		};
		FrameCommandBuffer Commands;

		AllocatedBuffer CamUniformBuffer;
		VkDescriptorSet GlobalDescriptorSet;

		AllocatedBuffer	ObjectBuffer;
		VkDescriptorSet ObjectDescriptorSet;
	};

	class VulkanRenderer : public RendererAPI
	{
	public:
		VulkanRenderer(const RendererSpecification& specs);
		~VulkanRenderer();

		void Init();
		void Shutdown();
		void OnResize(uint32_t width, uint32_t height);

		virtual inline uint32_t GetCurrentFrameIndex() const override { return m_CurrentImageIndex; }

		virtual void BeginFrame() override;
		//virtual void Draw(const std::vector<Renderable>& drawList) override;
		virtual void DrawRenderable(const Renderable& renderable) override;
		virtual void Draw() override;
		virtual void EndFrame() override;

		virtual inline const void* GetCommandBuffer(uint32_t index) override { return (const void* )GetFrame(index).Commands.Buffer; }
		virtual void BeginCommandBuffer(const void* cmd) override;
		virtual void EndCommandBuffer() override;

		virtual void BeginRenderPass(Ref<RenderPass> renderPass);
		virtual void EndRenderPass();

		virtual void BindPipeline(Ref<Pipeline> pipeline) override;

		virtual void Submit(const Renderable& object) override;

		virtual void PrepareSwapchainImage(Ref<Image2D> finalImage) override;


		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void UpdateCamera(const CameraUniform& cam) override;

		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();


		inline FrameData& GetCurrentFrame() { return m_Frames[m_CurrentFrame]; }

	private:
		FrameData& GetFrame(uint32_t index);
		void InitCommandControl();
		void InitFrameData();

		size_t PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		Ref<VulkanSwapchain> m_Swapchain = nullptr;
		SwapchainFrame* m_SwapchainFrame = nullptr;
		RendererSpecification m_Specification{};

		Scope<VulkanCommandControl> m_CommandControl = nullptr;
		Ref<UploadContext> m_UploadContext = nullptr;

		VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;
		Ref<VulkanRenderPass> m_ActiveRenderPass = nullptr;
		Ref<VulkanPipeline> m_ActivePipeline = nullptr;

		std::vector<FrameData> m_Frames;
		uint32_t m_CurrentFrame = 0;
		uint32_t m_CurrentImageIndex = 0;


		//------------- Scene and Object related ---------------
		VkDescriptorSetLayout m_GlobalDescriptorSetLayout;
		VkDescriptorSetLayout m_ObjectDescriptorSetLayout;

		std::unordered_map<Ref<VulkanShader>, std::vector<VkDescriptorSet>> m_DescriptorSetShaderMap;


		//Scene stuff
		Renderable* m_SceneObjects = nullptr;
		Renderable* m_SceneObjectPtr = nullptr;


		//SceneUniformBufferD m_SceneParameter;		// external
		//AllocatedBuffer m_SceneParameterBuffer;		// external


		static Scope<VulkanImGui> m_ImGui;

		struct VulkanRendererDebugInfo
		{
			uint64_t TotalFrames = 0;
		};
		VulkanRendererDebugInfo m_DebugInfo{};
	};

}
