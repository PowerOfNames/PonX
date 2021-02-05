#include "Sandbox2D.h"

#include "Platform/OpenGL/OpenGLShader.h"

#include <ImGui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Sandbox2D::Sandbox2D()
	:Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f, false)
{	
}

void Sandbox2D::OnAttach()
{
	PX_PROFILE_FUNCTION();


	m_TextureLogo = Povox::Texture2D::Create("assets/textures/logo.png");
}

void Sandbox2D::OnDetach()
{
	PX_PROFILE_FUNCTION();


	Povox::Renderer2D::Shutdown();
}

void Sandbox2D::OnUpdate(float deltatime)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnUpdate(deltatime);

	Povox::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
	Povox::RenderCommand::Clear();


	Povox::Renderer2D::BeginScene(m_CameraController.GetCamera());

 	Povox::Renderer2D::DrawQuad({ 1.0f, 1.0f }, { 0.5f, 0.5f }, m_SquareColor1);
	Povox::Renderer2D::DrawQuad({ 0.5f, -0.7f }, { 0.25f, 0.3f }, { 0.2f, 0.8f, 0.8f , 0.3f });
	Povox::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 2.0f, 2.0f }, m_TextureLogo, 2.0f, {1.0f, 0.5, 0.6f, 1.0f});

	//Povox::Renderer2D::DrawRotatedQuad({ 0.8f, -1.5f }, glm::radians(45.0), { 0.5f, 0.5f }, { 1.0f, 0.2f, 0.5f , 0.3f });

	Povox::Renderer2D::EndScene();
}

void Sandbox2D::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();


	ImGui::Begin("Square1");
	ImGui::ColorPicker4("Square1ColorPicker", &m_SquareColor1.r);
	ImGui::End();
}

void Sandbox2D::OnEvent(Povox::Event& e)
{
	m_CameraController.OnEvent(e);
}
