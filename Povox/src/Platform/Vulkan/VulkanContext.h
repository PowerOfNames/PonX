#pragma once

#include "Povox/Renderer/GraphicsContext.h"

#include "Povox/Core/Core.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanShader.h"
#include "VulkanBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

struct GLFWwindow;

namespace Povox {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;

		void RecreateSwapchain();

		//Framebuffer Callback
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

	private:
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
		void CreateFramebuffers();		

		void CreateDepthResources();
		bool HasStencilComponent(VkFormat format);

		void CreateTextureImage();
		void CreateTextureImageView();
		void CreateTextureSampler();
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		void CreateUniformBuffers();
		void UpdateUniformBuffer(uint32_t currentImage);

		// Commands
		void CreateCommandPool();
		void CreateCommandBuffers();
		VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool commandPool);
		void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

		// Semaphores
		void CreateSyncObjects();
		
		void CleanupSwapchain();

	private:
	// context specific
		GLFWwindow* m_WindowHandle;
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface;			// may get refactored out when dealing with multiple windows

		VkDebugUtilsMessengerEXT m_DebugMessenger;
	// external vk stuff
		Ref<VulkanDevice> m_Device;		// at the moment include both physical and logical device -> TODO: do seperate

	// swapchain
		Ref<VulkanSwapchain> m_Swapchain;

	// framebuffer
		std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	// renderpass
		Ref<VulkanRenderPass> m_RenderPass;

	//render pipelines
		Ref<VulkanPipeline> m_GraphicsPipeline;

	// Command buffer and pool
		VkCommandPool m_CommandPoolGraphics;
		VkCommandPool m_CommandPoolTransfer;

		std::vector<VkCommandBuffer> m_CommandBuffersGraphics;
		//VkCommandBuffer m_CommandBufferTransfer;

	// VK Buffer, memory and texture images + sampler
		VkImage m_TextureImage;
		VkImageView m_TextureImageView;
		VkDeviceMemory m_TextureImageMemory;
		VkSampler m_TextureSampler;

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;

		Ref<VulkanVertexBuffer> m_VertexBuffer;
		Ref<VulkanIndexBuffer> m_IndexBuffer;

		std::vector<VkBuffer> m_UniformBuffers;
		std::vector<VkDeviceMemory> m_UniformBuffersMemory;

		Ref<VulkanDescriptorPool> m_DescriptorPool;


	// sync objects
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFence;
		std::vector<VkFence> m_ImagesInFlight;
		uint32_t m_CurrentFrame = 0;

		bool m_FramebufferResized = false;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		
		const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	};
}
