#include <Povox.h>

#include <ImGui/imgui.h>


class ExampleLayer : public Povox::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{
		
	}

	void OnUpdate() override
	{
		if(Povox::Input::IsKeyPressed(PX_KEY_A))
			PX_TRACE("wquhhu!");

	}

	void OnEvent(Povox::Event& event) override
	{
		PX_TRACE("{0}", event);
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Test");
		ImGui::Text("Hello Povox User!");
		ImGui::End();
	}
};


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