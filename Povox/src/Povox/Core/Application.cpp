#include "pxpch.h"
#include "Povox/Core/Application.h"

#include "Povox/Core/Log.h"

#include "glad/glad.h"

namespace Povox {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this,  std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application() 
	{
		PX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
	}

	Application::~Application() 
	{
	}


	void Application::PushLayer(Layer* layer)
	{
		m_Layerstack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_Layerstack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_Layerstack.end(); it != m_Layerstack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			glClearColor(32.0f / 255, 93.0f / 255, 85.0f / 255, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			for (Layer* layer : m_Layerstack)
			{
				layer->OnUpdate();
			}

			m_Window->OnUpdate();

		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}
