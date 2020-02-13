#include "VoxelExample.h"

#include <ImGui/imgui.h>


VoxelExample::VoxelExample()
	:Layer("VoxelExample"), m_CameraController(1280.0f, 720.0f)
{

}

void VoxelExample::OnAttach()
{
	PX_PROFILE_FUNCTION();



}

void VoxelExample::OnDetach()
{
	PX_PROFILE_FUNCTION();



}

void VoxelExample::OnUpdate(float deltaTime)
{
	PX_PROFILE_FUNCTION();

	m_CameraController.OnUpdate(deltaTime);
	m_CameraPosition = m_CameraController.GetCamera().GetPosition();

	Povox::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
	Povox::RenderCommand::Clear();


	Povox::VoxelRenderer::BeginScene(m_CameraController.GetCamera());

	Povox::VoxelRenderer::DrawCube({ 0.0f, 0.0f, 0.0f}, { 1.0f, 1.0f, 1.0f }, {1.0f, 0.5f, 1.0f, 1.0f});

	Povox::VoxelRenderer::EndScene();

}

void VoxelExample::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();

	ImGui::Begin("Camera Settings");
	ImGui::SliderFloat3("CameraPosition", &m_CameraPosition.x, -100, 100);
	ImGui::End();

}

void VoxelExample::OnEvent(Povox::Event& e)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnEvent(e);
}
