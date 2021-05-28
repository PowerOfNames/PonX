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

		bool IsComplete() { return GraphicsFamily.has_value(); }
	};

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void* userData);


	private:
		void CreateInstance();

		//Debug
		void SetupDebugMessenger();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		// Physical Device
		void PickPhysicalDevice();
		int RateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		// Logical Device
		void CreateLogicalDevice();
		
		// Extensions
		void CheckRequiredExtensions(const char** extensions, uint32_t glfWExtensionsCount);
		std::vector<const char*> GetRequiredExtensions();
		bool CheckValidationLayerSupport();

	private:
		GLFWwindow* m_WindowHandle;
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_LogicalDevice;
		VkQueue m_GraphicsQueue;

		std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef PX_DEBUG
		const bool m_EnableValidationLayers = true;
#else
		const bool m_EnableValidationLayers = false;
#endif // 

	};

}
