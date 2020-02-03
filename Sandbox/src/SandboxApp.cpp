#include <Povox.h>
#include <Povox/Core/EntryPoint.h>

#include "ExampleLayer.h"
#include "Sandbox2D.h"

class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{
		PushLayer(new Sandbox2D());
		//PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{
	}
};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}
