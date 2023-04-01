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
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/TextureSystem.h"




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

		VkDescriptorSet TextureDescriptorSet;
	};

	class VulkanRenderer : public RendererAPI
	{
	public:
		VulkanRenderer(const RendererSpecification& specs);
		~VulkanRenderer();

		void Init();
		virtual void Shutdown();

		virtual inline uint32_t GetCurrentFrameIndex() const override { return m_CurrentFrameIndex; }
		
		virtual bool BeginFrame() override;
		virtual void DrawRenderable(const Renderable& renderable) override;
		virtual void Draw(Ref<Buffer> vertices, Ref<Buffer> indices, size_t indexCount) override;
		virtual void DrawGUI() override;
		virtual void EndFrame() override;

		virtual Ref<ShaderLibrary> GetShaderLibrary() override { return m_ShaderLibrary; }
		virtual Ref<TextureSystem> GetTextureSystem() override { return m_TextureSystem; }
				
		virtual void CreateFinalImage(Ref<Image2D> finalImage) override;

		virtual inline const void* GetCommandBuffer(uint32_t index) override { return (const void*)GetFrame(index).Commands.Buffer; }
		virtual inline const void* GetGUICommandBuffer(uint32_t index) override { return (const void*)(m_ImGui->GetFrame(index).CommandBuffer); }
		virtual void BeginCommandBuffer(const void* cmd) override;
		virtual void EndCommandBuffer() override;

		virtual void BeginRenderPass(Ref<RenderPass> renderPass) override;
		virtual void EndRenderPass() override;

		virtual void BeginGUIRenderPass() override;
		virtual void EndGUIRenderPass() override;

		virtual void BindPipeline(Ref<Pipeline> pipeline) override;

		virtual void Submit(const Renderable& object) override;

		virtual void PrepareSwapchainImage(Ref<Image2D> finalImage) override;

		virtual void FramebufferResized(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void UpdateCamera(const CameraUniform& cam) override;

		virtual inline Ref<Image2D> GetFinalImage() { return m_FinalImage; }

		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();
		virtual void* GetGUIDescriptorSet(Ref<Image2D> image) override;

	private:
		inline FrameData& GetCurrentFrame() { return m_Frames[m_CurrentFrameIndex]; }
		FrameData& GetFrame(uint32_t index);
		void InitCommandControl();
		void InitFrameData();
		void InitFinalImage(uint32_t width, uint32_t height);
		
		void CreateDescriptors();
		void CreateSamplers();

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
		uint32_t m_CurrentFrameIndex = 0;
		uint32_t m_CurrentSwapchainImageIndex = 0;

		bool m_FramebufferResized = false;
		uint32_t m_ViewportSizeX, m_ViewportSizeY = 0;

		//-------------			Samplers		 ---------------
		VkSampler m_TextureSampler = VK_NULL_HANDLE;

		Ref<VulkanImage2D> m_FinalImage = nullptr;

		//------------- Scene and Object related ---------------
		VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_ObjectDescriptorSetLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;

		std::unordered_map<Ref<VulkanShader>, std::vector<VkDescriptorSet>> m_DescriptorSetShaderMap;


		//Scene stuff
		Renderable* m_SceneObjects = nullptr;
		Renderable* m_SceneObjectPtr = nullptr;

		Ref<ShaderLibrary> m_ShaderLibrary;
		Ref<TextureSystem> m_TextureSystem;


		SceneUniform m_SceneParameter;
		AllocatedBuffer m_SceneParameterBuffer{};

		static Scope<VulkanImGui> m_ImGui;

		struct VulkanRendererDebugInfo
		{
			uint64_t TotalFrames = 0;
		};
		VulkanRendererDebugInfo m_DebugInfo{};
	};

}
