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

			VkSemaphore ComputeFinishedSemaphore;
		};
		FrameSemaphores Semaphores;
		VkFence RenderFence;
		VkFence ComputeFence;

		struct FrameCommandBuffer
		{
			VkCommandPool Pool;
			VkCommandBuffer RenderBuffer;
			VkCommandBuffer ComputeBuffer;
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

		// Core
		virtual bool Init() override;
		virtual void Shutdown() override;
		virtual void WaitForDeviceFinished() override;


		// FrameData
		virtual bool BeginFrame() override;
		virtual void Draw(Ref<Buffer> vertices, Ref<Material> material, Ref<Buffer> indices, size_t indexCount) override;
		virtual void DrawRenderable(const Renderable& renderable) override;
		virtual void EndFrame() override;
		virtual inline uint32_t GetCurrentFrameIndex() const override { return m_CurrentFrameIndex; }		
		virtual inline uint32_t GetLastFrameIndex() const override { return m_LastFrameIndex; }		

		virtual void PrepareSwapchainImage(Ref<Image2D> finalImage) override;
		virtual void CreateFinalImage(Ref<Image2D> finalImage) override;


		// State
		virtual void OnResize(uint32_t width, uint32_t height) override;
		virtual void OnViewportResize(uint32_t width, uint32_t height) override;
		virtual void OnSwapchainRecreate() override;
		virtual void UpdateCamera(const CameraUniform& cam) override;

		// Commands
		virtual void BeginCommandBuffer(const void* cmd) override;
		virtual void EndCommandBuffer() override;
		virtual inline const void* GetCommandBuffer(uint32_t index) override { return (const void*)GetFrame(index).Commands.RenderBuffer; }
		
		//Renderpass
		virtual void BeginRenderPass(Ref<RenderPass> renderPass) override;
		virtual void EndRenderPass() override;

		// Pipeline
		virtual void BindPipeline(Ref<Pipeline> pipeline) override;

		// Compute
		virtual void DispatchCompute(Ref<ComputePipeline> pipeline) override;

		// GUI
		virtual void BeginGUIRenderPass() override;
		virtual void DrawGUI() override;
		virtual void EndGUIRenderPass() override;
		virtual inline const void* GetGUICommandBuffer(uint32_t index) override { return (const void*)(m_ImGui->GetFrame(index).CommandBuffer); }
		virtual void* GetGUIDescriptorSet(Ref<Image2D> image) const override;
		// ImGui
		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();


		// Debugging and Performance
		virtual void StartTimestampQuery(const std::string& name) override;
		virtual void StopTimestampQuery(const std::string& name) override;


		// Resource Getters
		virtual Ref<ShaderLibrary> GetShaderLibrary() const override { return m_ShaderLibrary; }
		virtual Ref<TextureSystem> GetTextureSystem() const override { return m_TextureSystem; }
		virtual inline Ref<Image2D> GetFinalImage(uint32_t frameIndex) const override;

		virtual const RendererStatistics& GetStatistics() const override { return m_Statistics; }
		virtual inline const RendererSpecification& GetSpecification() const override { return m_Specification; }

	private:

		// FrameData
		void InitFrameData();
		inline FrameData& GetCurrentFrame() { return m_Frames[m_CurrentFrameIndex]; }
		FrameData& GetFrame(uint32_t index);

		// Resources
		void InitCommandControl();
		void InitFinalImage(uint32_t width, uint32_t height);
		
		// TEMP
		// Descriptors -> move to descriptor system
		void CreateDescriptors();
		void CreateSamplers();
		size_t PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment);
		// TEMP_END

		// Compute
		void ComputeSubmit();

		// Debugging and Performance
		void InitPerformanceQueryPools();
		void ResetQuerys(VkCommandBuffer cmd);
		void GetQueryResults();

	private:

		// StateInformation
		RendererSpecification m_Specification{};

		VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;
		Ref<VulkanRenderPass> m_ActiveRenderPass = nullptr;
		Ref<VulkanPipeline> m_ActivePipeline = nullptr;

		bool m_FramebufferResized = false, m_ViewportResized = false;
		uint32_t m_FramebufferWidth = 0, m_FramebufferHeight = 0, m_ViewportWidth = 0, m_ViewportHeight = 0;


		// FrameData
		SwapchainFrame* m_SwapchainFrame = nullptr;
		std::vector<FrameData> m_Frames;
		uint32_t m_CurrentFrameIndex = 0;
		uint32_t m_LastFrameIndex = 0;
		uint32_t m_CurrentSwapchainImageIndex = 0;
				

		// Resources
		Scope<VulkanCommandControl> m_CommandControl = nullptr;
		Ref<UploadContext> m_UploadContext = nullptr;

		Ref<ShaderLibrary> m_ShaderLibrary = nullptr;
		Ref<TextureSystem> m_TextureSystem = nullptr;

		std::vector<Ref<VulkanImage2D>> m_FinalImages;


		//TEMP
		// Samplers
		VkSampler m_TextureSampler = VK_NULL_HANDLE;


		// DescriptorSets
		VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_ObjectDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;

		std::unordered_map<Ref<VulkanShader>, std::vector<VkDescriptorSet>> m_DescriptorSetShaderMap;


		// Resources
		SceneUniform m_SceneParameter;
		AllocatedBuffer m_SceneParameterBuffer{};
		//TEMP_END




		// Debugging and Performance
		std::vector<VkQueryPool> m_TimestampQueryPools;
		std::vector<std::vector<uint64_t>> m_Timestamps;
		std::unordered_map<std::string, uint32_t> m_TimestampQueries;

		VkQueryPool m_PipelineStatisticsQueryPool = VK_NULL_HANDLE;

		RendererStatistics m_Statistics{};


		// Pointers
		VkDevice m_Device = VK_NULL_HANDLE;
		Ref<VulkanSwapchain> m_Swapchain = nullptr;
		static Scope<VulkanImGui> m_ImGui;

	};

}
