#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Povox {

	class Povosom : public Application
	{
	public:
		Povosom(const ApplicationSpecification& specs)
			: Application(specs)
		{
			PushLayer(new EditorLayer());
		}

		~Povosom()
		{
		}
	};

	Application* CreateApplication()
	{
		PX_TRACE("Start Povosom App!");

		ApplicationSpecification specs;



		specs.RendererProps.MaxFramesInFlight = 1;

		return new Povosom(specs);
	}

}
