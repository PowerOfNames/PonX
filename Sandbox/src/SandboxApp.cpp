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
		PX_TRACE("{0}", Povox::Input::IsKeyPressed(PX_KEY_A));
	}

	void OnEvent(Povox::Event& event) override
	{

		PX_TRACE("{0}", event);
		if (event.GetEventType() == Povox::EventType::KeyPressed)
		{
			PX_INFO("Button pressed");
			Povox::KeyPressedEvent& e = (Povox::KeyPressedEvent&)event;
			PX_TRACE("{0}", (char)e.GetKeyCode());
		}
	}
};


class Sandbox : public Povox::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		//PushOverlay(new Povox::ImGuiLayer());
	}

	~Sandbox()
	{

	}

};

Povox::Application* Povox::CreateApplication()
{
	return new Sandbox();
}