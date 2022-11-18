#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "ExampleLayer.h"
//#include "Sandbox2D.h"
//#include "VoxelExample.h"

class Sandbox : public Povox::Application
{
public:
	Sandbox(const Povox::ApplicationSpecification& specs)
		: Application(specs)
	{
		PushLayer(new ExampleLayer());
		//PushLayer(new Sandbox2D());
		//PushLayer(new VoxelExample());
	}

	~Sandbox()
	{
	}
};

Povox::Application* Povox::CreateApplication(int argc, char** argv)
{
	Povox::ApplicationSpecification specs;
	specs.UseAPI = Povox::RendererAPI::API::Vulkan;
	specs.ImGuiEnabled = true;
	

	return new Sandbox(specs);
}
