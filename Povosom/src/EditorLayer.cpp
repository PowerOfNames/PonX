#include "EditorLayer.h"

#include "Platform/OpenGL/OpenGLShader.h"

#include <ImGui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Povox {

    EditorLayer::EditorLayer()
        :Layer("Povosom2D"), m_CameraController(1280.0f / 720.0f, false)
    {
    }

    void EditorLayer::OnAttach()
    {
        PX_PROFILE_FUNCTION();

        FramebufferSpecification fbspec;
        fbspec.Width = 1280;
        fbspec.Height = 720;

        m_Framebuffer = Framebuffer::Create(fbspec);
        m_TextureLogo = Texture2D::Create("assets/textures/logo.png");
        m_SubTextureLogo = SubTexture2D::CreateFromCoords(m_TextureLogo, { 1, 2 }, { 64, 64 }, { 1, 2 });

        m_ActiveScene = CreateRef<Scene>();

        // Create entities
        Entity squareEntity = m_ActiveScene->CreateEntity("Square");
        squareEntity.AddComponent<SpriteRendererComponent>(glm::vec4{ 0.5f, 0.8f, 0.0f, 0.9f });

        m_SquareEntity = squareEntity;
    }

    void EditorLayer::OnDetach()
    {
        PX_PROFILE_FUNCTION();


        Renderer2D::Shutdown();
    }

    void EditorLayer::OnUpdate(Timestep deltatime)
    {
        PX_PROFILE_FUNCTION();


        m_Deltatime = deltatime;
        if(m_ViewportIsFocused)
            m_CameraController.OnUpdate(deltatime);

               
        Renderer2D::ResetStats();
        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
        RenderCommand::Clear();


        Renderer2D::BeginScene(m_CameraController.GetCamera());
        m_ActiveScene->OnUpdate(deltatime);
        Renderer2D::EndScene();


        m_Framebuffer->Unbind();
    }

    void EditorLayer::OnImGuiRender()
    {
        PX_PROFILE_FUNCTION();


        auto stats = Renderer2D::GetStats();


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
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

                if (ImGui::MenuItem("Exit"))
                    Application::Get().Close();

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Begin("Renderer2D Stats");
        ImGui::Text("DrawCalls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
        ImGui::Text("Deltatime: %f", m_Deltatime);
        ImGui::End();


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");

        m_ViewportIsFocused = ImGui::IsWindowFocused();
        m_ViewportIsHovered = ImGui::IsWindowHovered();
        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportIsFocused || !m_ViewportIsHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        if (m_ViewportSize != *((glm::vec2*)&viewportPanelSize) && (viewportPanelSize.x > 0) && (viewportPanelSize.y > 0))
        {
            m_Framebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
            m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

            m_CameraController.OnResize(viewportPanelSize.x, viewportPanelSize.y);
        }
        uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image((void*)textureID, ImVec2(m_ViewportSize.x, m_ViewportSize.y), { 0, 1 }, { 1, 0 });
        ImGui::End();
        ImGui::PopStyleVar(ImGuiStyleVar_WindowPadding);


        ImGui::Begin("Square1");

        if (m_SquareEntity)
        {
            auto& color = m_SquareEntity.GetComponent<SpriteRendererComponent>().Color;
            ImGui::ColorEdit4("Square1ColorPicker", glm::value_ptr(color));
        }
        
        ImGui::DragFloat2("Position", &m_SquarePos.x, 0.05f, -2.0f, 2.0f);
        ImGui::DragFloat2("Rotation Velocity", &m_RotationVel.x, 0.01f, -0.5f, 0.5f);
        ImGui::End();

        //Dockspace end
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& e)
    {
        m_CameraController.OnEvent(e);
    }

}
