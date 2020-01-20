#include <Povox.h>

class ExampleLayer : public Povox::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{

	}

	void OnUpdate() override
	{
		PX_INFO("ExampleLayer::OnUpdate");
	}

	void OnEvent(Povox::Event& event) override
	{
		PX_TRACE("{0}", event);
	}
};


class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushOverlay(new Povox::ImGuiLayer());
	}

	~Sandbox()
	{

	}

};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}