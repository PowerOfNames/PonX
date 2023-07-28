#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "SciSimLayer.h"


namespace Povox {

	class Povotom : public Application
	{
	public:
		Povotom(const ApplicationSpecification& specs)
			: Application(specs)
		{
			PushLayer(new SciSimLayer());
		}

		~Povotom()
		{
		}
	};

	Application* CreateApplication(int argc, char** argv)
	{
		PX_TRACE("Start Povotom App!");

		ApplicationSpecification specs;
		specs.UseAPI = RendererAPI::API::Vulkan;
		specs.ImGuiEnabled = true;
		specs.MaxFramesInFlight = 2;

		return new Povotom(specs);
	}

}
