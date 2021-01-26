#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "ExampleLayer.h"
#include "Sandbox2D.h"
#include "VoxelExample.h"
#include "TracerLayer.h"

class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{
		//PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
		//PushLayer(new VoxelExample());
		//PushLayer(new TracerLayer());
	}

	~Sandbox()
	{
	}
};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}
