#include "VoxelExample.h"

#include <ImGui/imgui.h>


VoxelExample::VoxelExample()
	:Layer("VoxelExample"), m_CameraController(1280.0f, 720.0f)
{
}

void VoxelExample::OnAttach()
{
	PX_PROFILE_FUNCTION();


	Povox::VoxelRenderer::Init();
}

void VoxelExample::OnDetach()
{
	PX_PROFILE_FUNCTION();


	Povox::VoxelRenderer::Shutdown();
}

void VoxelExample::OnUpdate(float deltaTime)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnUpdate(deltaTime);
	m_DeltaTime = deltaTime;

	Povox::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
	Povox::RenderCommand::Clear();

	Povox::VoxelRenderer::BeginScene(m_CameraController.GetCamera());

	Povox::VoxelRenderer::ResetStats();
	Povox::VoxelRenderer::BeginBatch();

	for (int i = 0; i < 1; i++)
	{		
		Povox::VoxelRenderer::DrawCube(glm::vec3( 0.0f, (float)i, 0.0f ), 1.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	}
	for (int i = 0; i < 1; i++)
	{
		Povox::VoxelRenderer::DrawCube(glm::vec3((float)i, 1.0f, 0.0f), 1.0f, "assets/textures/green.png");
	}

	Povox::VoxelRenderer::EndBatch();
	Povox::VoxelRenderer::Flush();

	Povox::VoxelRenderer::EndScene();
}

void VoxelExample::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnImGuiRender();

	ImGui::Begin("Renderer-Info");
	ImGui::Text("Cubes: %i", Povox::VoxelRenderer::GetStats().CubeCount);
	ImGui::Text("Draws: %i", Povox::VoxelRenderer::GetStats().DrawCount);
	ImGui::Text("Draws: %.4f", m_DeltaTime);
	ImGui::End();
}

void VoxelExample::OnEvent(Povox::Event& e)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnEvent(e);
}
