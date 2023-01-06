#include "pxpch.h"
#include "VulkanContext.h"

#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanDevice.h"


#include "Povox/Core/Application.h"
#include "Povox/Renderer/Renderer.h"



#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	Ref<VulkanDevice> VulkanContext::s_Device = nullptr;
	VkInstance VulkanContext::s_Instance = nullptr;
	VmaAllocator VulkanContext::s_Allocator = nullptr;
	Ref<VulkanDescriptorAllocator> VulkanContext::s_DescriptorAllocator = nullptr;
	Ref<VulkanDescriptorLayoutCache> VulkanContext::s_DescriptorLayoutCache = nullptr;

	std::vector<std::vector<std::function<void()>>> VulkanContext::s_ResourceFreeQueue;

	VulkanContext::VulkanContext()
	{
		CreateInstance();
#ifdef PX_ENABLE_VK_VALIDATION_LAYERS
		SetupDebugMessenger();
#endif
	}

	void VulkanContext::Init()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("VulkanContext: Startet Initialization!");

		// Device picking and creation -> happens automatically
		s_Device = CreateRef<VulkanDevice>();
		s_Device->PickPhysicalDevice(m_DeviceExtensions);
		m_PhysicalDeviceProperties = s_Device->GetPhysicalDeviceProperties();
		s_Device->CreateLogicalDevice(m_DeviceExtensions, m_ValidationLayers);
		PX_CORE_WARN("VulkanContext: Finished Core!");

		VmaAllocatorCreateInfo vmaAllocatorInfo = {};
		vmaAllocatorInfo.physicalDevice = s_Device->GetPhysicalDevice();
		vmaAllocatorInfo.device = s_Device->GetVulkanDevice();
		vmaAllocatorInfo.instance = s_Instance;
		vmaCreateAllocator(&vmaAllocatorInfo, &s_Allocator);
		PX_CORE_WARN("VulkanContext: Finished MemAllocator!");

		s_DescriptorAllocator = CreateRef<VulkanDescriptorAllocator>();
		s_DescriptorLayoutCache = CreateRef<VulkanDescriptorLayoutCache>();
		PX_CORE_TRACE("VulkanContext: created DecriptorAllocator and Layout Cache");

		s_ResourceFreeQueue.resize(Application::Get()->GetSpecification().MaxFramesInFlight);

		PX_CORE_TRACE("VulkanContext: Finished Initialization!");
	}

	void VulkanContext::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		for (auto& queue : s_ResourceFreeQueue)
		{
			for (auto& func : queue)
				func();
		}
		s_ResourceFreeQueue.clear();

		
		s_DescriptorAllocator->Cleanup();		
		s_DescriptorLayoutCache->Cleanup();
		

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
			DestroyDebugUtilsMessengerEXT(s_Instance, m_DebugMessenger, nullptr);
		vkDestroyInstance(s_Instance, nullptr);
	}


	void VulkanContext::CreateInstance()
	{
		if (PX_ENABLE_VK_VALIDATION_LAYERS && !CheckValidationLayerSupport())
		{
			PX_CORE_WARN("Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Povox Vulkan Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Povox";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (PX_ENABLE_VK_VALIDATION_LAYERS)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		PX_CORE_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &s_Instance), VK_SUCCESS, "Failed to create Vulkan Instance");
	}

	// Extensions and layers
	bool VulkanContext::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* vLayer : m_ValidationLayers)
		{
			//PX_CORE_INFO("Validation layer: '{0}'", vLayer);
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers)
			{
				//PX_CORE_INFO("available layers: '{0}'", layerProperties.layerName);

				if (strcmp(vLayer, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound) // return false at the first not found layer
				return false;
		}
		return true;
	}
	std::vector<const char*> VulkanContext::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (PX_ENABLE_VK_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		CheckRequiredExtensions(extensions);

		return extensions;
	}
	void VulkanContext::CheckRequiredExtensions(const std::vector<const char*>& extensions)
	{
		PX_CORE_INFO("Required Extensioncount: {0}", extensions.size());

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		for (uint32_t i = 0; i < extensions.size(); i++)
		{
			bool extensionFound = false;
			PX_CORE_INFO("Required Extension #{0}: {1}", i, extensions[i]);
			uint32_t index = 1;
			for (const auto& extension : availableExtensions)
			{
				if (strcmp(extensions[i], extension.extensionName))
				{
					extensionFound = true;
					PX_CORE_INFO("Extension requested and included {0}", extensions[i]);
					break;
				}
			}
			if (!extensionFound)
				PX_CORE_WARN("Extension requested but not included: {0}", extensions[i]);
		}
	}
	// Debug
	void VulkanContext::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		VkResult result = CreateDebugUtilsMessengerEXT(s_Instance, &createInfo, nullptr, &m_DebugMessenger);
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to set up debug messenger!");
	}
	void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	//by Cherno
	void VulkanContext::SubmitResourceFree(std::function<void()>&& func)
	{
		s_ResourceFreeQueue[Renderer::GetCurrentFrameIndex()].push_back(func);
	}
}
