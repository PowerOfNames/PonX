#pragma once

#include "Povox/Renderer/GraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Povox {

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;					// Does support windows

		bool IsComplete() { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		bool IsAdequat() { return !Formats.empty() && !PresentModes.empty(); }
	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void DrawFrame() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;


		// Debug
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData);

	private:
		void CreateInstance();

		// Debug
		void SetupDebugMessenger();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		// Surface
		void CreateSurface();

		// Physical Device
		void PickPhysicalDevice();
		int RateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		// Logical Device
		void CreateLogicalDevice();

		// Swapchain
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void CreateImageViews();
		
		// Renderpass
		void CreateRenderPass();

		// Graphics Pipeline
		void CreateGraphicsPipeline();
		VkShaderModule CreateShaderModule(const std::vector<char>& code);
		
		// Extensions
		void CreateSwapchain();
		void CheckRequiredExtensions(const char** extensions, uint32_t glfWExtensionsCount);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		// Framebuffers
		void CreateFramebuffers();

		// Commands
		void CreateCommandPool();
		void CreateCommandBuffers();

		// Semaphores
		void CreateSemaphores();

		std::vector<char> ReadFile(const std::string& filepath);
	private:
		GLFWwindow* m_WindowHandle;
		VkInstance m_Instance;
		VkSurfaceKHR m_Surface;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

		VkSwapchainKHR m_Swapchain;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_SwapchainExtent;

		std::vector<VkFramebuffer> m_SwapchainFramebuffers;

		VkRenderPass m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;

		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		VkSemaphore m_ImageAvailableSemaphore;
		VkSemaphore m_RenderFinishedSemaphore;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef PX_DEBUG
		const bool m_EnableValidationLayers = true;
#else
		const bool m_EnableValidationLayers = false;
#endif

	};
}
