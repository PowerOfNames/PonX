#pragma once
#include "Povox/Renderer/GraphicsContext.h"

#include "Povox/Core/Core.h"

#include "VulkanSwapchain.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"
#include "VulkanImGui.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

struct GLFWwindow;

namespace Povox {

	constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

	struct Texture
	{
		AllocatedImage Image;
		VkImageView ImageView;
	};

	struct FrameData
	{
		VkSemaphore PresentSemaphore, RenderSemaphore;
		VkFence RenderFence;

		VkCommandPool CommandPoolGfx;
		VkCommandBuffer MainCommandBuffer;
		//VkCommandPool CommandPoolTrsf;

		AllocatedBuffer CamUniformBuffer;

		VkDescriptorSet GlobalDescriptorSet;
	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext();
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;


		void OnFramebufferResize(uint32_t width, uint32_t height);

		void InitImGui();
		void BeginImGuiFrame();
		void EndImGuiFrame();
		void AddImGuiImage(float width, float height);

		void RecreateSwapchain();


	private:
		void CopyOffscreenToViewportImage(VkImage& swapchainImage);

	// stays here!
		// Instance
		void CreateInstance();
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		void CheckRequiredExtensions(const char** extensions, uint32_t glfWExtensionsCount);

		// Debug
		void SetupDebugMessenger();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		// Surface
		void CreateSurface();

	// Framebuffers
		void CreatePresentFramebuffers();		
		void CreateOffscreenFramebuffers();

		void CreateDepthResources();

		void CreateMemAllocator();
		void InitCommands();
		void InitDescriptors();
		void InitPipelines();

		void InitDefaultRenderPass();
		void InitOffscreenRenderPass();
		void CreateViewportImagesAndViews();
		void CreateTextureSampler();

		void UpdateUniformBuffer();


		// Semaphores
		void CreateSyncObjects();
		
		void CleanupSwapchain();

		void LoadMeshes();
		void UploadMeshes();

		void LoadTextures();

		size_t PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment);

		FrameData& GetCurrentFrame() { return m_Frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT]; }

	private:
	// context specific
		GLFWwindow* m_WindowHandle;				// may get refactored out when dealing with multiple windows
		VulkanCoreObjects m_CoreObjects;

		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

		VkDebugUtilsMessengerEXT m_DebugMessenger;

		Ref<VulkanSwapchain> m_Swapchain;

		FrameData m_Frames[MAX_FRAMES_IN_FLIGHT];
		UploadContext m_UploadContext;

		SceneUniformBufferD m_SceneParameter;
		AllocatedBuffer m_SceneParameterBuffer;

		VkDescriptorSet m_PresentImGuiSet{ VK_NULL_HANDLE };
		ImTextureID m_PresentImGui = nullptr;
		bool m_PresentImGuiAlive = false;

		bool m_NeedsResize = false;
		bool m_Resized = false;

		std::vector<VulkanFramebuffer> m_OffscreenFramebuffers{};
		std::vector<VkFramebuffer> m_SwapchainFramebuffers{};

		VkRenderPass m_DefaultRenderPass;
		VkRenderPass m_OffscreenRenderPass;

		VkPipeline m_GraphicsPipeline;
		VkPipeline m_LastPipeline;
		VkPipelineLayout m_GraphicsPipelineLayout;


		Scope<VulkanImGui> m_ImGui;

	// VK Buffer, memory and texture images + sampler
		Mesh m_DoubleTriangleMesh;

		std::unordered_map<std::string, Texture> m_Textures;

		VkSampler m_TextureSampler;

		AllocatedImage m_DepthImage;
		VkImageView m_DepthImageView;

		VkDescriptorSetLayout m_GlobalDescriptorSetLayout;
		VkDescriptorPool m_DescriptorPool;

		
		AllocatedImage m_ViewportImage;
		VkImageView m_ViewportImageView;


		bool m_FramebufferResized = false;
		int m_WindowWidth, m_WindowHeight;


	// sync objects

		uint32_t m_CurrentFrame = 0;

		bool m_GuiPipelineEnabled = true;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};
}
