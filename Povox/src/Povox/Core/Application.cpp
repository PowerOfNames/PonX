#include "pxpch.h"
#include "Povox/Core/Application.h"

#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Core/Log.h"

#include "GLFW/glfw3.h"

namespace Povox {

	Application::Application() 
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
	}

	Application::~Application() 
	{
	
	}

	void Application::Run()
	{
		while (m_Running)
		{
			glClearColor(1.0f, 0.0f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			m_Window->OnUpdate();
		}
	}
}
