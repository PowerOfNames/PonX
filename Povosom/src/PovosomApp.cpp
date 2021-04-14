#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Povox {

	class Povosom : public Application
	{
	public:
		Povosom()
		{
			PushLayer(new EditorLayer());
		}

		~Povosom()
		{
		}
	};

	Application* CreateApplication()
	{
		return new Povosom();
	}

}
