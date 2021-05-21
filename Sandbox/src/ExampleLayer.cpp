#include "ExampleLayer.h"

#include <ImGui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


ExampleLayer::ExampleLayer()
	: Layer("Example")
{	
}

void ExampleLayer::OnAttach()
{
}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate(Povox::Timestep deltatime)
{
	Povox::RenderCommand::SetClearColor({ 0.15f, 0.16f, 0.15f, 1.0f });
	Povox::RenderCommand::Clear();
	

}


void ExampleLayer::OnImGuiRender()
{
	
}

void ExampleLayer::OnEvent(Povox::Event& e)
{
}