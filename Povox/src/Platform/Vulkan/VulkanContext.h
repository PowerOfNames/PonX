#pragma once
#include "Povox/Renderer/GraphicsContext.h"

#include "Povox/Core/Core.h"

#include "VulkanDevice.h"
#include "VulkanImGui.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#pragma warning(push, 0)
#include <vulkan/vulkan.h>
#pragma warning(pop)
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>



namespace Povox {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext();

		virtual void Init() override;
		virtual void Shutdown() override;


		static const Ref<VulkanDevice> GetDevice() { return s_Device; }
		static const VkInstance GetInstance() { return s_Instance; }
		static const VkDescriptorPool GetDescriptorPool() { return s_DescriptorPool; }
		static const VmaAllocator GetAllocator() { return s_Allocator; }
		static std::vector<std::vector<std::function<void()>>>& GetResourceFreeQueue() { return s_ResourceFreeQueue; }

		static void SubmitResourceFree(std::function<void()>&& func);

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

	// Framebuffers	
		void CreateOffscreenFramebuffers();


		void CreateMemAllocator();
		void InitDescriptorPool();

		void InitOffscreenRenderPass();
		void CreateViewportImagesAndViews();

		
		void CleanupSwapchain();

		void UploadMeshes();

		void LoadTextures();

		

		

	private:
	// context specific
		//static ContextSpecification? s_Specs -> GetSepc()->Device->GetLogic... or something
		//	|
		//	-->Context static, globally available. no more use for statioc CoreObjects and inits everywhere
		static Ref<VulkanDevice> s_Device;
		static VkInstance s_Instance;
		static VmaAllocator s_Allocator;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;										//			| ContextSpec			
		
		//by Cherno
		static std::vector<std::vector<std::function<void()>>> s_ResourceFreeQueue;

		VkDebugUtilsMessengerEXT m_DebugMessenger;
		
		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };



		static VkDescriptorPool s_DescriptorPool;

		// all imgui will evantually be refactored out to use the normal engine render code instead of being inside the context
		
		VkDescriptorSet m_PresentImGuiSet{ VK_NULL_HANDLE };
		ImTextureID m_PresentImGui = nullptr;
		bool m_PresentImGuiAlive = false;



		VkDescriptorSetLayout m_GlobalDescriptorSetLayout;
		VkDescriptorSetLayout m_ObjectDescriptorSetLayout;
		VkDescriptorPool m_DescriptorPool;

	};
}
