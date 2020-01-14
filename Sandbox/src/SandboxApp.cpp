#include <Povox.h>

class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}

};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}