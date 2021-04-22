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
	m_SubTextureLogo = Povox::SubTexture2D::CreateFromCoords(m_TextureLogo, { 1, 2 }, { 64, 64 }, { 1, 2 });

}

void Sandbox2D::OnDetach()
{
	PX_PROFILE_FUNCTION();


	Povox::Renderer2D::Shutdown();
}

void Sandbox2D::OnUpdate(Povox::Timestep deltatime)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnUpdate(deltatime);

	Povox::Renderer2D::ResetStats();
    {
        PX_PROFILE_SCOPE("Renderer_Preps")
	    Povox::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
	    Povox::RenderCommand::Clear();
    }

	static float rotation = 0.0;
	rotation += deltatime * 20;

	static float x = 0.0f;
	static float y = 0.0f;

	x = glm::cos(rotation * m_RotationVel.x) * 1.2;
	y = glm::sin(rotation * m_RotationVel.y) * 1.2;

	Povox::Renderer2D::BeginScene(m_CameraController.GetCamera());
 	Povox::Renderer2D::DrawQuad(m_SquarePos, { 0.5f, 0.5f }, m_SquareColor1);
	Povox::Renderer2D::DrawQuad({ 0.5f, -0.7f }, { 0.25f, 0.3f }, { 0.2f, 0.8f, 0.8f , 0.3f });
	Povox::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 2.0f, 2.0f }, m_TextureLogo, 10.0f, {1.0f, 0.5, 0.6f, 1.0f});
	Povox::Renderer2D::DrawQuad({ 0.5f, 0.5f, 0.1f }, { 0.5f, 1.0f }, m_SubTextureLogo);

	Povox::Renderer2D::DrawRotatedQuad({ -0.8f, -1.0f }, { 0.5f, 0.5f }, glm::radians(45.0f), m_TextureLogo);
	Povox::Renderer2D::DrawRotatedQuad({ x + m_SquarePos.x, y + m_SquarePos.y}, { 0.5f, 0.5f }, glm::radians(rotation * 3), { x, y, x , 0.3f });
	Povox::Renderer2D::EndScene();
}

void Sandbox2D::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();


	auto stats = Povox::Renderer2D::GetStats();

	ImGui::Begin("Renderer2D Stats");
    ImGui::Text("DrawCalls: %d", stats.DrawCalls);
    ImGui::Text("Quads: %d", stats.QuadCount);
    ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
    ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
    ImGui::End();


    ImGui::Begin("Square1");
    ImGui::ColorPicker4("Square1ColorPicker", &m_SquareColor1.r);
    ImGui::DragFloat2("Position", &m_SquarePos.x, 0.05f, -2.0f, 2.0f);
    ImGui::DragFloat2("Rotation Velocity", &m_RotationVel.x, 0.01f, -0.5f, 0.5f);
    ImGui::End();

}

void Sandbox2D::OnEvent(Povox::Event& e)
{
	m_CameraController.OnEvent(e);
}
