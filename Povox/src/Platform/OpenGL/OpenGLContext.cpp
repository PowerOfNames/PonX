#include "pxpch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Povox {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		PX_CORE_ASSERT(windowHandle, "Window handle is null");
	}

	void OpenGLContext::Init()
	{
		PX_PROFILE_FUNCTION();


		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		PX_CORE_ASSERT(status, "Failed to initialize Glad!");

		PX_CORE_INFO("--- Povox Info: ---");
		PX_CORE_INFO(" Vendor   : {0}", glGetString(GL_VENDOR));
		PX_CORE_INFO(" Renderer : {0}", glGetString(GL_RENDERER));
		PX_CORE_INFO(" Version  : {0}", glGetString(GL_VERSION));


		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		PX_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor > 5), "Povox requires at least version 4.5!");

	}

	void OpenGLContext::Shutdown()
	{

	}

	void OpenGLContext::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();


		glfwSwapBuffers(m_WindowHandle);
	}
}