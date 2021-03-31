#include "Povosom2D.h"

#include "Platform/OpenGL/OpenGLShader.h"

#include <ImGui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

    Povosom2D::Povosom2D()
        :Layer("Povosom2D"), m_CameraController(1280.0f / 720.0f, false)
    {
    }

    void Povosom2D::OnAttach()
    {
        PX_PROFILE_FUNCTION();

        Povox::FramebufferSpecification fbspec;
        fbspec.Width = 1280;
        fbspec.Height = 720;

        m_Framebuffer = Povox::Framebuffer::Create(fbspec);
        m_TextureLogo = Povox::Texture2D::Create("assets/textures/logo.png");
        m_SubTextureLogo = Povox::SubTexture2D::CreateFromCoords(m_TextureLogo, { 1, 2 }, { 64, 64 }, { 1, 2 });

    }

    void Povosom2D::OnDetach()
    {
        PX_PROFILE_FUNCTION();


        Povox::Renderer2D::Shutdown();
    }

    void Povosom2D::OnUpdate(float deltatime)
    {
        PX_PROFILE_FUNCTION();


        m_CameraController.OnUpdate(deltatime);

        Povox::Renderer2D::ResetStats();
        {
            PX_PROFILE_SCOPE("Renderer_Preps")
                m_Framebuffer->Bind();
            Povox::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1.0f });
            Povox::RenderCommand::Clear();
        }

        static float rotation = 0.0;
        rotation += deltatime * 20;

        static float x = 0.0f;
        static float y = 0.0f;

        x = glm::cos(rotation * m_RotationVel.x) * 1.2;
        y = glm::sin(rotation * m_RotationVel.y) * 1.2;

        //PX_INFO("X = {0}", x);
        //PX_INFO("Y = {0}", y);

        Povox::Renderer2D::BeginScene(m_CameraController.GetCamera());

        Povox::Renderer2D::DrawQuad(m_SquarePos, { 0.5f, 0.5f }, m_SquareColor1);
        Povox::Renderer2D::DrawQuad({ 0.5f, -0.7f }, { 0.25f, 0.3f }, { 0.2f, 0.8f, 0.8f , 0.3f });
        Povox::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 2.0f, 2.0f }, m_TextureLogo, 10.0f, { 1.0f, 0.5, 0.6f, 1.0f });
        Povox::Renderer2D::DrawQuad({ 0.5f, 0.5f, 0.1f }, { 0.5f, 1.0f }, m_SubTextureLogo);

        Povox::Renderer2D::DrawRotatedQuad({ -0.8f, -1.0f }, { 0.5f, 0.5f }, 45.0f, m_TextureLogo);
        Povox::Renderer2D::DrawRotatedQuad({ x + m_SquarePos.x, y + m_SquarePos.y }, { 0.5f, 0.5f }, rotation * 3, { x, y, x , 0.3f });

        Povox::Renderer2D::EndScene();

        m_Framebuffer->Unbind();
    }

    void Povosom2D::OnImGuiRender()
    {
        PX_PROFILE_FUNCTION();


        auto stats = Povox::Renderer2D::GetStats();


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
                    Povox::Application::Get().Close();

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Begin("Renderer2D Stats");
        ImGui::Text("DrawCalls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

        uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image((void*)textureID, { 1280, 960 }, { 0, 1 }, { 1, 0 });
        ImGui::End();


        ImGui::Begin("Square1");
        ImGui::ColorPicker4("Square1ColorPicker", &m_SquareColor1.r);
        ImGui::DragFloat2("Position", &m_SquarePos.x, 0.05f, -2.0f, 2.0f);
        ImGui::DragFloat2("Rotation Velocity", &m_RotationVel.x, 0.01f, -0.5f, 0.5f);
        ImGui::End();

        //Dockspace end
        ImGui::End();
    }

    void Povosom2D::OnEvent(Povox::Event& e)
    {
        m_CameraController.OnEvent(e);
    }

}
