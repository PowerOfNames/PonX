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



namespace Povox {

	struct Texture
	{
		AllocatedImage Image2D;
		VkImageView ImageView;
	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext();

		virtual void Init() override;
		virtual void Shutdown() override;


		void InitImGui();
		void BeginImGuiFrame();
		void EndImGuiFrame();
		void AddImGuiImage(float width, float height);

		void RecreateSwapchain();

		static const Ref<VulkanDevice> GetDevice() { return s_Device; }
		static const VkInstance GetInstance() { return s_Instance; }
		static const VkDescriptorPool GetDescriptorPool() { return s_DescriptorPool; }
		static const VmaAllocator GetAllocator() { return s_Allocator; }

	private:
		void CopyOffscreenToViewportImage(VkImage& swapchainImage);
		void SwapBuffers();
	// stays here!
		// Instance
		void CreateInstance();
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		void CheckRequiredExtensions(const char** extensions, uint32_t glfWExtensionsCount);

		// Debug
		void SetupDebugMessenger();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	// Framebuffers	
		void CreateOffscreenFramebuffers();

		void CreateDepthResources();

		void CreateMemAllocator();
		void InitCommands();
		void InitDescriptorPool();

		void InitOffscreenRenderPass();
		void CreateViewportImagesAndViews();

		void UpdateUniformBuffer();


		// Semaphores
		void CreateSyncObjects();
		
		void CleanupSwapchain();

		void LoadMeshes();
		void UploadMeshes();

		void LoadTextures();

		size_t PadUniformBuffer(size_t inputSize, size_t minGPUBufferOffsetAlignment);

		FrameData& GetCurrentFrameIndex() { return m_Frames[m_CurrentFrame % MAX_FRAMES_IN_FLIGHT]; }

	private:
	// context specific
		//static ContextSpecification? s_Specs -> GetSepc()->Device->GetLogic... or something
		//	|
		//	-->Context static, globally available. no more use for statioc CoreObjects and inits everywhere
		static Ref<VulkanDevice> s_Device;
		static VkInstance s_Instance;
		static VmaAllocator s_Allocator;
		static VkDescriptorPool s_DescriptorPool;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;										//			| ContextSpec			


		SceneUniformBufferD m_SceneParameter;		// external
		AllocatedBuffer m_SceneParameterBuffer;		// external

		// all imgui will evantually be refactored out to use the normal engine render code instead of bein inside the context
		
		VkDescriptorSet m_PresentImGuiSet{ VK_NULL_HANDLE };
		ImTextureID m_PresentImGui = nullptr;
		bool m_PresentImGuiAlive = false;


		VulkanFramebufferPool m_FramebufferPool;		// maybe not needed -> why useful, if I pass the framebuffer via the renderpass needed to do a renderpass
		std::vector<VulkanFramebuffer> m_OffscreenFramebuffers{};	// external



		AllocatedImage m_DepthImage;					// external
		VkImageView m_DepthImageView;					// external

		AllocatedImage m_ViewportImage;					// external
		VkImageView m_ViewportImageView;				// external


		VkDebugUtilsMessengerEXT m_DebugMessenger;

	// VK Buffer, memory and texture images + sampler
		Mesh m_DoubleTriangleMesh;						// external refactor

		std::unordered_map<std::string, Texture> m_Textures;	// external

		VkDescriptorSetLayout m_GlobalDescriptorSetLayout;
		VkDescriptorSetLayout m_ObjectDescriptorSetLayout;
		VkDescriptorPool m_DescriptorPool;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};
}
