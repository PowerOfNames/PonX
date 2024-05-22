#pragma once
#include "Platform/Vulkan/VulkanCommands.h"

#include "Platform/Vulkan/VulkanImGui.h"

#include "Platform/Vulkan/VulkanMaterialSystem.h"
#include "Platform/Vulkan/VulkanShaderResourceSystem.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "Platform/Vulkan/VulkanSwapchain.h"

#include "Povox/Core/Application.h"
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/RendererAPI.h"

#include "Povox/Resources/ShaderManager.h"

#include "Povox/Systems/TextureSystem.h"




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

			VkCommandPool ComputePool;
			VkCommandBuffer ComputeBuffer;
		};
		FrameCommandBuffer Commands;

		AllocatedBuffer CamUniformBuffer;
		VkDescriptorSet GlobalDescriptorSet;

		AllocatedBuffer	ObjectBuffer;
		VkDescriptorSet ObjectDescriptorSet;

		AllocatedBuffer ParticleBuffer;
		VkDescriptorSet ParticleDescriptorSet;

		VkDescriptorSet TextureDescriptorSet;
	};

	class VulkanQueryManager;
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
		virtual void Draw(Ref<Buffer> vertices, uint64_t firstVertexOffset, Ref<Material> material, Ref<Buffer> indices, size_t indexCount, bool textureless) override;
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
		virtual void DispatchCompute(Ref<ComputePass> computePass, uint64_t totalElements, uint32_t workGroupWeightX, uint32_t workGroupWeightY, uint32_t workGroupWeightZ) override;

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
		virtual void AddTimestampQuery(const std::string& name, uint32_t count) override;
		virtual void AddPipelineStatisticsQuery(const std::string& name, const std::string& computeStatQueryPoolName) override;
		virtual inline const std::string& GetComputeStatisticsQueryPoolName() const override { return m_ComputeStatisticsQueryPoolName; }

		// Resource Getters
		virtual inline Ref<ShaderManager> GetShaderManager() const override { return m_ShaderManager; }
		virtual inline Ref<TextureSystem> GetTextureSystem() const override { return m_TextureSystem; }
		virtual inline Ref<MaterialSystem> GetMaterialSystem() const override { return m_MaterialSystem; }
		virtual inline Ref<ShaderResourceSystem> GetShaderResourceSystem() const override { return m_ShaderResourceSystem; }
		virtual inline Ref<Image2D> GetFinalImage(uint32_t frameIndex) const override;

		virtual inline const RendererStatistics& GetStatistics() const override { return m_Statistics; }
		virtual inline const RendererSpecification& GetSpecification() const override { return m_Specification; }

	private:

		// FrameData
		void InitFrameData();
		inline FrameData& GetCurrentFrame() { return m_Frames[m_CurrentFrameIndex]; }
		FrameData& GetFrame(uint32_t index);
		bool PrepareRenderFrame();
		bool PrepareComputeFrame();

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
		void GetQueryResults(uint32_t frameIdx);

	private:

		// StateInformation
		RendererSpecification m_Specification{};

		VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;
		Ref<VulkanRenderPass> m_ActiveRenderPass = nullptr;
		Ref<VulkanPipeline> m_ActivePipeline = nullptr;
		
		Ref<VulkanComputePass> m_ActiveComputePass = nullptr;


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

		Ref<ShaderManager> m_ShaderManager = nullptr;
		Ref<TextureSystem> m_TextureSystem = nullptr;
		Ref<VulkanMaterialSystem> m_MaterialSystem = nullptr;
		Ref<VulkanShaderResourceSystem> m_ShaderResourceSystem = nullptr;

		std::vector<Ref<VulkanImage2D>> m_FinalImages;


		//TEMP
		// Samplers
		VkSampler m_TextureSampler = VK_NULL_HANDLE;


		// DescriptorSets
		VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_ObjectDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayout m_ParticleDescriptorSetLayout = VK_NULL_HANDLE;

		std::unordered_map<Ref<VulkanShader>, std::vector<VkDescriptorSet>> m_DescriptorSetShaderMap;


		// Resources
		SceneUniform m_SceneParameter;
		AllocatedBuffer m_SceneParameterBuffer{};
		//TEMP_END




		// Debugging and Performance
		Ref<VulkanQueryManager> m_QueryManager = nullptr;		
		std::string m_ComputeStatisticsQueryPoolName = "ComputeQueryPool";

		RendererStatistics m_Statistics{};


		// Pointers
		VkDevice m_Device = VK_NULL_HANDLE;
		Ref<VulkanSwapchain> m_Swapchain = nullptr;
		static Scope<VulkanImGui> m_ImGui;

	};

}
