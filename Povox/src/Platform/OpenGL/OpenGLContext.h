#pragma once

#include "Povox/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Povox {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void Shutdown() override;

		virtual void DrawFrame() override {};
	private:
		GLFWwindow* m_WindowHandle;
	};
}
