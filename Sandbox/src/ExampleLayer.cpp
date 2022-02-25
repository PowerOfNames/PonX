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
	// Create the main framebuffer
	/*Povox::FramebufferSpecification fbspec;
	fbspec.Attachements = { Povox::FramebufferTextureFormat::RGBA8, Povox::FramebufferTextureFormat::RED_INTEGER, Povox::FramebufferTextureFormat::Depth };
	fbspec.Width = 1280.0f;
	fbspec.Height = 720.0f;
	m_Framebuffer = Povox::Framebuffer::Create(fbspec);*/

	m_EditorCamera = Povox::EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

}

void ExampleLayer::OnDetach()
{
	//Renderer shutdown
}

void ExampleLayer::OnUpdate(Povox::Timestep deltatime)
{

	//Set framebuffer size
	Povox::RenderCommand::SetClearColor({ 0.15f, 0.16f, 0.15f, 1.0f });
	Povox::RenderCommand::Clear();

	//BeginBatch(Scene)
	//add stuff
	//endbatch(scene)
}

void ExampleLayer::OnImGuiRender()
{
	PX_CORE_WARN("Begin ExampleLayer On ImGui render");

	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float miWinDockspace = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	style.WindowMinSize.x = miWinDockspace;

 // Viewport
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin(" Viewport ");

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewPortOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewPortOffset.x, viewportMinRegion.y + viewPortOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewPortOffset.x, viewportMaxRegion.y + viewPortOffset.y };

		m_ViewportIsFocused = ImGui::IsWindowFocused();
		m_ViewportIsHovered = ImGui::IsWindowHovered();
		Povox::Application::Get().GetImGuiVulkanLayer()->BlockEvents(!m_ViewportIsFocused || !m_ViewportIsHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		//uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID(0);
		//ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		Povox::RenderCommand::AddImGuiImage(m_ViewportSize.x, m_ViewportSize.y);
		ImGui::End();
		ImGui::PopStyleVar();
	}

	ImGui::Begin("Test Tab");
	ImGui::Checkbox("Demo ImGui", &m_DemoActive);
	if (m_DemoActive)
		ImGui::ShowDemoWindow();
	ImGui::End();

	ImGui::End(); //Dockspace end
}

void ExampleLayer::OnEvent(Povox::Event& e)
{
}
