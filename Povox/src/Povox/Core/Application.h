#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Core/Window.h"
#include "Povox/Events/Event.h"

namespace Povox {

	class POVOX_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	// To be defined in CLIENT	
	Application* CreateApplication();

}
