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

	Application* CreateApplication(int argc, char** argv)
	{
		PX_TRACE("Start Povosom App!");

		ApplicationSpecification specs;
		specs.UseAPI = RendererAPI::API::Vulkan;
		specs.ImGuiEnabled = true;
		specs.MaxFramesInFlight = 2;

		return new Povosom(specs);
	}

}
