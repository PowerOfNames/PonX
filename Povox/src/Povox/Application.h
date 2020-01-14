#pragma once

#include "Core.h"

namespace Povox {

	class POVOX_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// To be defined in CLIENT	
	Application* CreateApplication();

}
