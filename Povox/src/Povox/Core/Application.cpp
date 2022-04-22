#include "pxpch.h"
#include "Povox/Core/Application.h"

#include "Povox/Core/Log.h"
#include "Povox/Core/Input.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/RendererAPI.h"
#include "Povox/Core/Timestep.h"

#include <GLFW/glfw3.h>


namespace Povox {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specs)
	{
		PX_PROFILE_FUNCTION();

		s_Specification = specs;

		PX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		//TODO: Implement OnStartup() -> sets up rendererContext, which renderer to use, creates window etc.
		//Set Graphics API to Vulkan when available, else to OpenGL
		RendererAPI::SetAPI(RendererAPI::API::Vulkan);

		// Window holds scope of the graphics context and initializes it upon window init (which happens in the constructor)
		m_Window = Window::Create();
		m_Window->SetEventCallback(PX_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();

		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::OpenGL:
			{
				m_ImGuiLayer = new ImGuiLayer();
				PushOverlay(m_ImGuiLayer);
				break;
			}
			case RendererAPI::API::Vulkan:
			{
				m_ImGuiVulkanLayer = new ImGuiVulkanLayer();
				PushOverlay(m_ImGuiVulkanLayer);
				break;
			}
			default:
			PX_CORE_ASSERT(false, "This API is not supported!");
		}		
	}

	Application::~Application() 
	{
	}

	void Application::PushLayer(Layer* layer)
	{
		PX_PROFILE_FUNCTION();


		m_Layerstack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		PX_PROFILE_FUNCTION();


		m_Layerstack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		PX_PROFILE_FUNCTION();


		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(PX_BIND_EVENT_FN(Application::OnWindowClose)); 
		dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_Layerstack.rbegin(); it != m_Layerstack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		PX_PROFILE_FUNCTION();


		while (m_Running)
		{
			PX_PROFILE_SCOPE("Application Run-Loop");

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_DeltaTime;
			m_DeltaTime = time;

			if (!m_Minimized)
			{
				for (Layer* layer : m_Layerstack)
				{
					layer->OnUpdate(timestep);
				}
			}

			if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
			{
				m_ImGuiLayer->Begin();
				for (Layer* layer : m_Layerstack)
				{
					layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				m_ImGuiVulkanLayer->Begin();
				for (Layer* layer : m_Layerstack)
				{
					layer->OnImGuiRender();
				}
				m_ImGuiVulkanLayer->End();
			}

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		PX_PROFILE_FUNCTION();

		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();


		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
		m_Window->OnResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	bool Application::OnFramebufferResize(FramebufferResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();


		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnFramebufferResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void Application::Close()
	{
		m_Running = false;
	}
}
