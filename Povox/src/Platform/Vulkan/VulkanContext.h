#pragma once

#include "Povox/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Povox {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;
	};

}
