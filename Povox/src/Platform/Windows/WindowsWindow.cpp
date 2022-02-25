#include "pxpch.h"
#include "Platform/Windows/WindowsWindow.h"

#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Events/MouseEvent.h"
#include "Povox/Events/KeyEvent.h"

#include "Povox/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanRendererAPI.h"


namespace Povox {

	// static, because we may have more then one window, but want to initialize GLFW only once;
	static uint8_t s_GLFWwindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		PX_CORE_ERROR("GLFW error ({0}): {1}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsWindow::~WindowsWindow() 
	{
		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		PX_PROFILE_FUNCTION();


		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		PX_CORE_INFO("Creating window '{0}' ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

		if (s_GLFWwindowCount == 0)
		{
			PX_PROFILE_SCOPE("GLFW init!");
			int success = glfwInit();
			PX_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		
			//PX_PROFILE_SCOPE("GLFW create window!");
		#ifdef PX_DEBUG
			if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
		#endif

			if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
			{
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
				m_GLFW_NO_API = true;
			}

		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, props.Title.c_str(), nullptr, nullptr);
		++s_GLFWwindowCount;		
		
		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();
		//VulkanRendererAPI::SetContext(std::dynamic_pointer_cast<VulkanContext>(m_Context));

		glfwSetWindowUserPointer(m_Window, &m_Data);
		if(!m_GLFW_NO_API)
			SetVSync(true);

		// Set GLFW callbacks
	// --- Window
		// Window resizing
		//return window size in screen cordinates
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		//returns window size in pixels
		glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				FramebufferResizeEvent event(width, height);
				data.EventCallback(event);
			});

		// Window closing
		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

	// --- Keys
		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
					{
					case GLFW_PRESS: 
					{
						KeyPressedEvent event(key, 0);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE: 
					{
						KeyReleasedEvent event(key);
						data.EventCallback(event);
						break;
					}
					case GLFW_REPEAT:
					{
						KeyPressedEvent event(key, 1);
						data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event(keycode);
				data.EventCallback(event);
			});

	// --- Mouse
		// Mouse Button
		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				PX_PROFILE_FUNCTION();


				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		// Mouse Scrolling
		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				PX_PROFILE_FUNCTION();


				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		// Mouse Position
		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				PX_PROFILE_FUNCTION();


				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
	}

	void WindowsWindow::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		glfwDestroyWindow(m_Window);
		--s_GLFWwindowCount;

		if (s_GLFWwindowCount == 0)
		{
			m_Context->Shutdown();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		PX_PROFILE_FUNCTION();

		
		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		PX_PROFILE_FUNCTION();

		
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);
		

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}
}
