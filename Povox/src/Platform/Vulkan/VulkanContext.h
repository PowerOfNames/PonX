#pragma once

#include "Povox/Renderer/GraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

struct GLFWwindow;

namespace Povox {

	struct UniformBufferObject
	{
		alignas(16) glm::mat4 ModelMatrix;
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;					// Does support windows
		std::optional<uint32_t> TransferFamily;					// not supported by all GPUs

		bool IsComplete() { return GraphicsFamily.has_value() && PresentFamily.has_value(); }
		bool HasTransfer() { return TransferFamily.has_value(); }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		bool IsAdequat() { return !Formats.empty() && !PresentModes.empty(); }
	};

	struct VertexData
	{
		glm::vec2 Position;
		glm::vec3 Color;

		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription vertexBindingDescription{};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = sizeof(VertexData);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return vertexBindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> vertexAttributeDescriptions{};
			vertexAttributeDescriptions[0].binding = 0;
			vertexAttributeDescriptions[0].location = 0;
			vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributeDescriptions[0].offset = offsetof(VertexData, Position);

			vertexAttributeDescriptions[1].binding = 0;
			vertexAttributeDescriptions[1].location = 1;
			vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexAttributeDescriptions[1].offset = offsetof(VertexData, Color);

			return vertexAttributeDescriptions;
		}
	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void DrawFrame() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;

		void RecreateSwapchain();

		//Framebuffer Callback
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

		// Debug
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData);

	private:
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
		
		// Physical Device
		void PickPhysicalDevice();
		int RateDeviceSuitability(VkPhysicalDevice device);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		// Logical Device
		void CreateLogicalDevice();
		
		// Swapchain
		void CreateSwapchain();
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		
		// Image Views
		void CreateImageViews();

		// Renderpass
		void CreateRenderPass();

		// Graphics Pipeline
		void CreateDescriptorSetLayout();
		void CreateGraphicsPipeline();
		VkShaderModule CreateShaderModule(const std::vector<char>& code);
		std::vector<char> ReadFile(const std::string& filepath);

		// Framebuffers
		void CreateFramebuffers();
		
		// Commands
		void CreateCommandPool();

		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateUniformBuffers();

		void CreateDescriptorPool();
		void CreateDescriptorSets();

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
			VkBuffer& buffer, VkDeviceMemory& memory, uint32_t familyIndexCount = 0, uint32_t* familyIndices = nullptr, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
		void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertiyFlags);
		void CreateCommandBuffers();

		// Semaphores
		void CreateSyncObjects();

		
		void CleanupSwapchain();

		void UpdateUniformBuffer(uint32_t currentImage);

	private:
		GLFWwindow* m_WindowHandle;
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkSurfaceKHR m_Surface;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkQueue m_TransferQueue;


		VkSwapchainKHR m_Swapchain;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_SwapchainExtent;

		std::vector<VkFramebuffer> m_SwapchainFramebuffers;

		VkRenderPass m_RenderPass;

		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;

		VkCommandPool m_CommandPoolGraphics;
		VkCommandPool m_CommandPoolTransfer;

		VkBuffer m_VertexBuffer;
		VkDeviceMemory m_VertexBufferMemory;
		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;
		std::vector<VkBuffer> m_UniformBuffers;
		std::vector<VkDeviceMemory> m_UniformBuffersMemory;

		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;

		std::vector<VkCommandBuffer> m_CommandBuffersGraphics;
		VkCommandBuffer m_CommandBufferTransfer;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFence;
		std::vector<VkFence> m_ImagesInFlight;
		uint32_t m_CurrentFrame = 0;

		bool m_FramebufferResized = false;

		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		
		const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		const std::vector<VertexData> m_Vertices = {
			{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
			{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
		};

		const std::vector<uint16_t> m_Indices = {
			 0, 1, 2, 2, 3, 0
		};


//#ifdef PX_DEBUG
		const bool m_EnableValidationLayers = true;
//#else
	//	const bool m_EnableValidationLayers = false;
//#endif

	};
}
