#include "pxpch.h"
#include "VulkanContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

namespace Povox {

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		PX_CORE_ASSERT(windowHandle, "Window handle is null");
	}

	void VulkanContext::Init()
	{
		PX_PROFILE_FUNCTION();
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		PX_CORE_INFO("ExtensionCount: {0}", extensionCount);

	}

	void VulkanContext::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();



	}


}