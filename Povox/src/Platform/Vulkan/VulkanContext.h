#pragma once

#include "Povox/Renderer/GraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Povox {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;

	private:
		void CreateInstance();
		void CheckRequiredExtensions(const char** extensions, uint32_t glfWExtensionsCount);

	private:
		GLFWwindow* m_WindowHandle;
		VkInstance m_Instance;
	};

}
