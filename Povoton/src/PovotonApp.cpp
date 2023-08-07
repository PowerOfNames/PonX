#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "SciSimLayer.h"


namespace Povox {

	class Povoton : public Application
	{
	public:
		Povoton(const ApplicationSpecification& specs)
			: Application(specs)
		{
			PushLayer(new SciSimLayer());
		}

		~Povoton()
		{
		}
	};

	Application* CreateApplication(int argc, char** argv)
	{
		PX_TRACE("Start Povoton App!");

		ApplicationSpecification specs;
		specs.UseAPI = RendererAPI::API::Vulkan;
		specs.ImGuiEnabled = true;
		specs.MaxFramesInFlight = 2;

		return new Povoton(specs);
	}

}
