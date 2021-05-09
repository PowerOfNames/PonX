#include "pxpch.h"

#include "Povox/ImGui/ImGuiLayer.h"

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include "Povox/Core/Application.h"

//TEMPORARY
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ImGuizmo.h>

namespace Povox {

	ImGuiLayer::ImGuiLayer()
		:Layer("ImGuiLayer")
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSansJP/NotoSansJP-Bold.otf", 18.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSansJP/NotoSansJP-Regular.otf", 18.0f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		SetDarkThemeColors();

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_crrent_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_crrent_context);
		}
	}

	void ImGuiLayer::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.13f, 0.13f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.18f, 0.21f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.22f, 0.25f, 0.25f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.22f, 0.25f, 0.25f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.14f, 0.17f, 0.17f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.22f, 0.25f, 0.25f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.22f, 0.25f, 0.25f, 1.0f };

		// Frame BG (fields)
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.35f, 0.38f, 0.38f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.39f, 0.42f, 0.42f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.39f, 0.42f, 0.42f, 1.0f };

		colors[ImGuiCol_SliderGrab] = ImVec4{ 1.0f, 0.11f, 0.11f, 1.0f };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 1.0f, 0.11f, 0.11f, 1.0f };

		colors[ImGuiCol_CheckMark] = ImVec4{ 0.1f, 0.5f, 0.2f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.1f, 0.13f, 0.13f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.22f, 0.25f, 0.25f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.19f, 0.22f, 0.22f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.09f, 0.12f, 0.12f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.14f, 0.16f, 0.16f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.05f, 0.08f, 0.08f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.1f, 0.13f, 0.13f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.05f, 0.08f, 0.08f, 1.0f };

		// Docking
		colors[ImGuiCol_DockingEmptyBg] = ImVec4{ 0.1f, 0.25f, 0.15f, 1.0f };
		colors[ImGuiCol_DockingPreview] = ImVec4{ 0.14f, 0.29f, 0.19f, 1.0f };

		// Borders
		colors[ImGuiCol_Border] = ImVec4{ 0.05f, 0.08f, 0.08f, 1.0f };
		colors[ImGuiCol_BorderShadow] = ImVec4{ 0.05f, 0.08f, 0.08f, 0.5f };

		// Popup
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.08f, 0.11f, 0.11f, 1.0f };

		// Resizes
		colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.1f, 0.25f, 0.15f, 0.5f };
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.14f, 0.29f, 0.19f, 1.0f };
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.14f, 0.29f, 0.19f, 1.0f };

		// Separator
		colors[ImGuiCol_Separator] = ImVec4{ 0.1f, 0.25f, 0.15f, 0.5f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.14f, 0.29f, 0.19f, 1.0f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.14f, 0.29f, 0.19f, 1.0f };

		// Text
		colors[ImGuiCol_Text] = ImVec4{ 0.9f, 0.9f, 0.9f, 0.9f };
		colors[ImGuiCol_TextSelectedBg] = ImVec4{ 0.3f, 0.33f, 0.33f, 1.0f };
		colors[ImGuiCol_TextDisabled] = ImVec4{ 0.35f, 0.35f, 0.35f, 1.0f };
	}

	void ImGuiLayer::OnImGuiRender()
	{
	}

}