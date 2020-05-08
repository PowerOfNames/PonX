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
	Povox::RenderCommand::SetDrawMode(m_DrawMode);

	Povox::VoxelRenderer::BeginScene(m_CameraController.GetCamera());

	Povox::VoxelRenderer::ResetStats();
	Povox::VoxelRenderer::BeginBatch();

	for (int i = 0; i < m_Size; i++)
	{
		for (int j = 0; j < m_Size; j++)
		{
			for (int k = 0; k < m_Size; k++)
			{
				Povox::VoxelRenderer::DrawCube(glm::vec3((float)i, (float)j, (float)k), 1.0f, glm::vec4(32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f));
			}
		}
	}
	//Povox::VoxelRenderer::DrawCube({0.0f, 5.0f, 0.0f}, 1.0f, "assets/textures/green.png");


	Povox::VoxelRenderer::EndBatch();
	Povox::VoxelRenderer::Flush();

	Povox::VoxelRenderer::EndScene();
}

void VoxelExample::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnImGuiRender();

	ImGui::Begin("Renderer-Info");
	ImGui::Checkbox("Toggle Draw Mode", &m_DrawMode);
	ImGui::SliderInt("Size: ", &m_Size, 0, 100);
	ImGui::Text("Cubes: %i", Povox::VoxelRenderer::GetStats().CubeCount);
	ImGui::Text("Draws: %i", Povox::VoxelRenderer::GetStats().DrawCount);
	ImGui::Text("Triangles: %i", Povox::VoxelRenderer::GetStats().TriangleCount);
	ImGui::Text("FPS: %.4f", 1/m_DeltaTime);
	ImGui::End();
}

void VoxelExample::OnEvent(Povox::Event& e)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnEvent(e);
}
