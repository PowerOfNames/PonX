#include "pxpch.h"
#include "Platform/Windows/WindowsWindow.h"

#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Events/MouseEvent.h"
#include "Povox/Events/KeyEvent.h"

#include "Povox/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanRenderer.h"


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

		PX_CORE_INFO("WindowsWindow::Init: Starting initialization...");
		PX_CORE_INFO("Window name: '{0}'; Initial size: ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

		if (s_GLFWwindowCount == 0)
		{
			PX_PROFILE_SCOPE("GLFW init!");
			int success = glfwInit();
			PX_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE:
			{
				PX_CORE_ASSERT(true, "RendererAPI has not been set yes!");
			}
			case RendererAPI::API::Vulkan:
			{
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
				m_GLFW_NO_API = true;
			}
		}

		PX_CORE_INFO("Creating glfwWindow handle...");
		m_Window = glfwCreateWindow(static_cast<int>(props.Width), static_cast<int>(props.Height), props.Title.c_str(), nullptr, nullptr);
		++s_GLFWwindowCount;	
		PX_CORE_INFO("Window count: {0}", s_GLFWwindowCount);
		PX_CORE_INFO("Completed glfwWindow handle creation.");
		
		//Create Context with vkinstance 
		m_Context = GraphicsContext::Create();
		//use vkinstance to create surface in swapchain
		m_Swapchain = CreateRef<VulkanSwapchain>(m_Window);
		//init context with surface after swapchain has set surface up
		m_Context->Init();
		//init rest of swapchain
		m_Swapchain->Init(static_cast<uint32_t>(m_Data.Width), static_cast<uint32_t>(m_Data.Height));


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

		PX_CORE_INFO("WindowsWindow::Init: Completed initialization.");
	}

	void WindowsWindow::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		if(m_Swapchain)
			m_Swapchain->Destroy(); // check if this should happen after windowCountCheck -> mutliple swapchains for multiple windows possibly?
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
		m_Swapchain->SwapBuffers();
	}

	void WindowsWindow::OnResize(uint32_t width, uint32_t height)
	{
		m_Swapchain->FramebufferResized();
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
