#include <Povox.h>

#include <Povox/Core/EntryPoint.h>

#include "ExampleLayer.h"

class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{
	}
};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}
