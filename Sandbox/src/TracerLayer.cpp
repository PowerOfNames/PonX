#include "TracerLayer.h"

#include <ImGui/imgui.h>


TracerLayer::TracerLayer()
	:Layer("TracerLayer"), m_CameraController(1280.0f, 720.0f)
{
}

void TracerLayer::OnAttach()
{
	PX_PROFILE_FUNCTION();


	Povox::RayTracer::Init();
}

void TracerLayer::OnDetach()
{
	PX_PROFILE_FUNCTION();


	Povox::RayTracer::Shutdown();
}

void TracerLayer::OnUpdate(float deltaTime)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnUpdate(deltaTime);
	m_DeltaTime = deltaTime;

	Povox::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
	Povox::RenderCommand::Clear();

	Povox::RayTracer::BeginScene(m_CameraController.GetCamera());

	Povox::RayTracer::Trace(m_CameraController.GetCamera());

	Povox::RayTracer::EndScene();
}

void TracerLayer::OnImGuiRender()
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnImGuiRender();

	ImGui::Begin("Renderer-Info");
	ImGui::Text("FPS: %.4f", 1 / m_DeltaTime);
	ImGui::End();
}

void TracerLayer::OnEvent(Povox::Event& e)
{
	PX_PROFILE_FUNCTION();


	m_CameraController.OnEvent(e);
}