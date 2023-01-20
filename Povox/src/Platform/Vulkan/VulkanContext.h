#pragma once
#include "Povox/Renderer/GraphicsContext.h"

#include "Povox/Core/Core.h"

#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanImGui.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#pragma warning(push, 0)
#include <vulkan/vulkan.h>
#pragma warning(pop)
#include <vk_mem_alloc.h>


#include <glm/glm.hpp>



namespace Povox {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext();

		virtual void Init() override;
		virtual void Shutdown() override;


		static Ref<VulkanDevice> GetDevice() { return s_Device; }
		static VkInstance GetInstance() { return s_Instance; }
		static Ref<VulkanDescriptorAllocator> GetDescriptorAllocator() { return s_DescriptorAllocator; }
		static Ref<VulkanDescriptorLayoutCache> GetDescriptorLayoutCache() { return s_DescriptorLayoutCache; }
		static VmaAllocator GetAllocator() { return s_Allocator; }
		static std::vector<std::vector<std::function<void()>>>& GetResourceFreeQueue() { return s_ResourceFreeQueue; }

		static void SubmitResourceFree(std::function<void()>&& func);

	private:
	// stays here!
		// Instance
		void CreateInstance();
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		void CheckRequiredExtensions(const std::vector<const char*>& glfwExtensions);

		// Debug
		void SetupDebugMessenger();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	private:
		static Ref<VulkanDevice> s_Device;
		static VkInstance s_Instance;
		static VmaAllocator s_Allocator;
		
		//by Cherno
		static std::vector<std::vector<std::function<void()>>> s_ResourceFreeQueue;

		static Ref<VulkanDescriptorAllocator> s_DescriptorAllocator;
		static Ref<VulkanDescriptorLayoutCache> s_DescriptorLayoutCache;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		
		const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};
}
