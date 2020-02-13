#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "ExampleLayer.h"
#include "Sandbox2D.h"
#include "VoxelExample.h"

class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{
		//PushLayer(new Sandbox2D());
		//PushLayer(new ExampleLayer());
		PushLayer(new VoxelExample());
	}

	~Sandbox()
	{
	}
};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}
