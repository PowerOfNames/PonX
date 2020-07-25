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

	Povox::Renderer2D::Init();


	m_MapData = Povox::Texture2D::Create("MapData", 8, 8);
	uint32_t mapData[64];
	for (unsigned int i = 0; i < 64; i++)
	{
		mapData[i] = 0xffffffff;
	}
	mapData[0] = 0x00000000;
	mapData[63] = 0x00000000;
	m_MapData->SetData(&mapData, sizeof(uint32_t) * 64);
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

	Povox::Renderer2D::DrawQuad({ 1.0f, 1.0f }, { 1.0f, 1.0f }, "assets/textures/green.png");
	Povox::Renderer2D::DrawQuad({ -1.0f, -1.0f }, { 1.0f, 1.0f }, m_MapData);
	Povox::Renderer2D::DrawQuad(m_SquarePos1, { 1.0f, 1.0f }, m_SquareColor1);

	Povox::Renderer2D::EndScene();
}

void Sandbox2D::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();


	ImGui::Begin("Square1");
	ImGui::ColorPicker4("Square1ColorPicker", &m_SquareColor1.r);
	ImGui::SliderFloat2("Square1Position", &m_SquarePos1.x, -5.0f, 5.0f);
	ImGui::End();
}

void Sandbox2D::OnEvent(Povox::Event& e)
{
	m_CameraController.OnEvent(e);
}
