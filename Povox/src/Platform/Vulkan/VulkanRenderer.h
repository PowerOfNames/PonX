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

		// Core
		virtual void Shutdown();


		// FrameData
		virtual bool BeginFrame() override;
		virtual void Draw(Ref<Buffer> vertices, Ref<Material> material, Ref<Buffer> indices, size_t indexCount) override;
		virtual void DrawRenderable(const Renderable& renderable) override;
		virtual void Submit(const Renderable& object) override;
		virtual void EndFrame() override;
		virtual inline uint32_t GetCurrentFrameIndex() const override { return m_CurrentFrameIndex; }		

		virtual void PrepareSwapchainImage(Ref<Image2D> finalImage) override;
		virtual void CreateFinalImage(Ref<Image2D> finalImage) override;


		// State
		virtual void UpdateCamera(const CameraUniform& cam) override;
		virtual void FramebufferResized(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		// Commands
		virtual void BeginCommandBuffer(const void* cmd) override;
		virtual void EndCommandBuffer() override;
		virtual inline const void* GetCommandBuffer(uint32_t index) override { return (const void*)GetFrame(index).Commands.Buffer; }
		
		//Renderpass
		virtual void BeginRenderPass(Ref<RenderPass> renderPass) override;
		virtual void EndRenderPass() override;

		// Pipeline
		virtual void BindPipeline(Ref<Pipeline> pipeline) override;


		// GUI
		virtual void BeginGUIRenderPass() override;
		virtual void DrawGUI() override;
		virtual void EndGUIRenderPass() override;
		virtual inline const void* GetGUICommandBuffer(uint32_t index) override { return (const void*)(m_ImGui->GetFrame(index).CommandBuffer); }
		virtual void* GetGUIDescriptorSet(Ref<Image2D> image) override;
		// ImGui
		static void InitImGui();
		static void BeginImGuiFrame();
		static void EndImGuiFrame();


		// Debugging and Performance
		virtual void StartTimestampQuery(const std::string& name) override;
		virtual void StopTimestampQuery(const std::string& name) override;


		// Resource Getters
		virtual Ref<ShaderLibrary> GetShaderLibrary() override { return m_ShaderLibrary; }
		virtual Ref<TextureSystem> GetTextureSystem() override { return m_TextureSystem; }
		virtual inline Ref<Image2D> GetFinalImage() { return m_FinalImage; }

		virtual const RendererStatistics& GetStatistics() const override { return m_Statistics; }
		virtual inline const RendererSpecification& GetSpecification() override { return m_Specification; }

	private:
		//Core
		void Init();

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

		bool m_FramebufferResized = false;
		uint32_t m_ViewportSizeX, m_ViewportSizeY = 0;


		// FrameData
		SwapchainFrame* m_SwapchainFrame = nullptr;
		std::vector<FrameData> m_Frames;
		uint32_t m_CurrentFrameIndex = 0;
		uint32_t m_CurrentSwapchainImageIndex = 0;
				

		// Resources
		Scope<VulkanCommandControl> m_CommandControl = nullptr;
		Ref<UploadContext> m_UploadContext = nullptr;

		Ref<ShaderLibrary> m_ShaderLibrary;
		Ref<TextureSystem> m_TextureSystem;

		Ref<VulkanImage2D> m_FinalImage = nullptr;


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
