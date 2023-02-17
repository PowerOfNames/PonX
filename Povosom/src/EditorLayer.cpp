#include "EditorLayer.h"

#include "Platform/OpenGL/OpenGLShader.h"

#include <ImGui/imgui.h>
#include <ImGuizmo.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Povox/Utils/PlatformsUtils.h"
#include "Povox/Scene/SceneSerializer.h"
#include "Povox/Math/Math.h"

namespace Povox {

    EditorLayer::EditorLayer()
        : Layer("Povosom2D"), m_OrthoCamControl(1.778f, false)
    {
    }

    void EditorLayer::OnAttach()
    {
        PX_PROFILE_FUNCTION();

		m_LogoTexture = Texture2D::Create("assets/textures/logo.png");

		PX_CORE_TRACE("Started EditorLayer::OnAttach()");
		//ImGuiOfflineRendering
		{
			FramebufferSpecification imGuiViewportFBSpecs{};
			imGuiViewportFBSpecs.Attachments = { {ImageFormat::RGBA8}, {ImageFormat::RED_INTEGER, MemoryUtils::MemoryUsage::GPU_TO_CPU}, {ImageFormat::Depth} };
			imGuiViewportFBSpecs.Width = Application::Get()->GetWindow().GetWidth();
			imGuiViewportFBSpecs.Height = Application::Get()->GetWindow().GetHeight();
			imGuiViewportFBSpecs.SwapChainTarget = false;
			m_ImGuiViewportFB = Framebuffer::Create(imGuiViewportFBSpecs);


			RenderPassSpecification imGuiViewportRPSpecs{};
			imGuiViewportRPSpecs.TargetFramebuffer = m_ImGuiViewportFB;
			imGuiViewportRPSpecs.ColorAttachmentCount = 2;
			imGuiViewportRPSpecs.HasDepthAttachment = true;
			m_ImGuiRenderpass = RenderPass::Create(imGuiViewportRPSpecs);
		}

		//Flatcolored Pipeline
		{
			PipelineSpecification flatColoredPipelineSpecs{};
			flatColoredPipelineSpecs.DynamicViewAndScissors = true;
			flatColoredPipelineSpecs.Culling = PipelineUtils::CullMode::NONE;
			flatColoredPipelineSpecs.TargetRenderPass = m_ImGuiRenderpass;
			flatColoredPipelineSpecs.Shader = Renderer::GetShaderLibrary()->Get("FlatColorShader");
			m_FlatColorPipeline = Pipeline::Create(flatColoredPipelineSpecs);
		}

		//Textured Pipeline
		{
			PipelineSpecification texturedPipelineSpecs{};
			texturedPipelineSpecs.DynamicViewAndScissors = true;
			texturedPipelineSpecs.Culling = PipelineUtils::CullMode::NONE;
			texturedPipelineSpecs.TargetRenderPass = m_ImGuiRenderpass;
			texturedPipelineSpecs.Shader = Renderer::GetShaderLibrary()->Get("TextureShader");
			m_TexturePipeline = Pipeline::Create(texturedPipelineSpecs);
		}

        m_ActiveScene = CreateRef<Scene>();

        m_EditorCamera = EditorCamera(60.0f, 1.778f, 0.1f, 1000.0f);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

		PX_CORE_TRACE("Finished EditorLayer::OnAttach()");
    }


    void EditorLayer::OnDetach()
    {
        PX_PROFILE_FUNCTION();


        Renderer2D::Shutdown();
    }


    void EditorLayer::OnUpdate(Timestep deltatime)
    {
        PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("EditorLayer::OnUpdate Started!");
        m_Deltatime = deltatime;

        //Resize
        if (FramebufferSpecification spec = m_ImGuiViewportFB->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && //zero sized framebuffer is invalid
            (spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
        {
			//m_ImGuiViewportFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y); //beter resize all pipelines here -> can then recursively 
			//resize the attached renderpass and framebuffers
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);

            m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        }

        //Update
        m_EditorCamera.OnUpdate(deltatime);
		m_OrthoCamControl.OnUpdate(deltatime);

        //Renderer Stats
        Renderer2D::ResetStats();

        // Update Scene
        m_ActiveScene->OnUpdateEditor(deltatime, m_EditorCamera);

		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);
		Renderer::BeginCommandBuffer(cmd);
		Renderer::BeginRenderPass(m_ImGuiRenderpass);

		//Renderer2D::BeginScene(m_EditorCamera); //doesn't mess with renderer atm
		Renderer2D::BeginScene(m_OrthoCamControl.GetCamera()); //doesn't mess with renderer atm

		Renderer::BindPipeline(m_FlatColorPipeline);// -> this will be moved into sceneRenderer -> sorts through materials of objects, then binds pipeline accordingly
		Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.56f, 0.24f, 0.5f, 0.7f });	
		Renderer2D::DrawRotatedQuad({ 1.0f, 1.0f, 0.5f }, { 1.0f, 1.0f }, 0.5f, { 0.9f, 0.4f, 0.5f, 1.0f });	
		Renderer2D::DrawQuad({ 0.0f, 0.0f, 4.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 0.5f, 0.2f });
		
		Renderer::BindPipeline(m_TexturePipeline);
		Renderer2D::DrawQuad({ 2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, m_LogoTexture, 0.5f);

		Renderer2D::EndScene(); //doesn't mess with renderer atm

		Renderer::EndRenderPass();
		Renderer::EndCommandBuffer();

		//CopyFinalImage into current SwapchainImage
		if (!Application::Get()->GetSpecification().ImGuiEnabled)
			Renderer::PrepareSwapchainImage(m_ImGuiViewportFB->GetColorAttachment(0));
		else
			Renderer::CreateFinalImage(m_ImGuiViewportFB->GetColorAttachment(0));

        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        my = viewportSize.y - my;
        int mouseX = (int)mx;
        int mouseY = (int)my;
        
        if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
        {
            //int pixelData = m_ImGuiViewportFB->GetColorAttachment(1)->ReadPixel(1, mouseX, mouseY);
            //m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
        }
		PX_CORE_TRACE("EditorLayer::OnUpdate Finished!");
    }

    void EditorLayer::OnImGuiRender()
    {
        PX_PROFILE_FUNCTION();

		PX_CORE_TRACE("EditorLayer::OnImguiRender Started!");

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
        ImGuiStyle& style = ImGui::GetStyle();
        float miWinDockspace = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = miWinDockspace;

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

                if (ImGui::MenuItem("New", "Ctrl+N"))
                    NewScene();

                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                    OpenScene();

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                    SaveScene();

                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                    SaveSceneAs();

                if (ImGui::MenuItem("Exit", "Ctrl+Esc"))
                    CloseApp();

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

     // Renderer Stats
        ImGui::Begin(" Stats ");
        ImGui::Text("DrawCalls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
        ImGui::Text("Deltatime: %f", m_Deltatime);

        std::string name = "None";
        if (m_HoveredEntity)
            name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
        ImGui::Text("Hovered Entity: %s", name.c_str());

        ImGui::End(); // Renderer Stats


     // Viewport
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin(" Viewport ");

        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewPortOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewPortOffset.x, viewportMinRegion.y + viewPortOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x+ viewPortOffset.x, viewportMaxRegion.y + viewPortOffset.y };

        m_ViewportIsFocused = ImGui::IsWindowFocused();
        m_ViewportIsHovered = ImGui::IsWindowHovered();
        Application::Get()->GetImGuiVulkanLayer()->BlockEvents(!m_ViewportIsFocused || !m_ViewportIsHovered);


        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		PX_CORE_TRACE("EditorLayer::OnImguiRender Pre Viewport!");
		ImTextureID texID = Renderer::GetGUIDescriptorSet(Renderer::GetFinalImage());
        ImGui::Image(texID, ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
        
		PX_CORE_TRACE("EditorLayer::OnImguiRender Post Viewport!");

     // Gizmos
        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && m_GizmoType != -1)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

            // Runtime camera
            //auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            //
            //const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            //const glm::mat4& cameraProjection = camera.GetProjection();
            //glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());
            
            // Editor Camera            
            const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
            glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

            // Entity Transform
            auto& tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            m_GizmoSnap = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
            float snapValue = 0.5f;
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.0f;

            float snapValues[] = { snapValue, snapValue, snapValue };

            ImGui::Begin("Gizmos");
            ImGui::SameLine();
            switch ((ImGuizmo::OPERATION)m_GizmoType)
            {
            case ImGuizmo::TRANSLATE:
                ImGui::InputFloat3("Snap", &snapValues[0]);
                break;
            case ImGuizmo::ROTATE:
                ImGui::InputFloat3("Angle Snap", &snapValues[1]);
                break;
            case ImGuizmo::SCALE:
                ImGui::InputFloat3("Scale Snap", &snapValues[2]);
                break;
            }

            ImGui::End();
            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), 
                                    (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, m_GizmoSnap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);
                glm::vec3 deltaRotation = rotation - tc.Rotation;
                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }

        }

        ImGui::End();
		//ImGui::PopStyleVar(ImGuiStyleVar_WindowPadding);
		ImGui::PopStyleVar();
               

        ImGui::Begin("Test Tab");
        ImGui::Checkbox("Demo ImGui", &m_DemoActive);
        if(m_DemoActive)
            ImGui::ShowDemoWindow();
        ImGui::End();

     // Panels
        m_SceneHierarchyPanel.OnImGuiRender();

        ImGui::Begin("Editor Camera");
        glm::vec3 cameraPos = m_EditorCamera.GetPostion();
        ImGui::Text("Camera Position: {%f|%f|%f}", cameraPos.x, cameraPos.y, cameraPos.z);
        ImGui::End();

        ImGui::End(); //Dockspace end

		PX_CORE_TRACE("EditorLayer::OnImguiRender Finished!");
    }

    void EditorLayer::OnEvent(Event& e)
    {
        m_EditorCamera.OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(PX_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(PX_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        //shortcuts
        if (e.GetRepeatCount() > 0)
            return false;
        bool controlPressed = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shiftPressed = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
        switch (e.GetKeyCode())
        {
            case Key::N:
            {
                if (controlPressed)
                {
                    NewScene();
                    break;
                }
            }
            case Key::O:
            {
                if (controlPressed)
                {
                    OpenScene();
                    break;
                }
            }
            case Key::S:
            {
                if (controlPressed)
                {
                    if (shiftPressed)
                    {
                        SaveSceneAs();
                        break;
                    }
                    SaveScene();
                    break;
                }
            }
            case Key::Q:
            {
                m_GizmoType = -1;
                break;
            }
            case Key::W:
            {
                m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Key::E:
            {
                m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Key::R:
            {
                m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            }
            case Key::Escape:
            {
                if (controlPressed)
                {
                    CloseApp();
                    break;
                }
            }
        }
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        bool isAltPressed = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
        switch (e.GetMouseButton())
        {
            case Mouse::ButtonLeft:
            {
                if(!isAltPressed && !ImGuizmo::IsOver() && m_ViewportIsHovered)
                    m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
                break;
            }
        }

        return false;
    }

    void EditorLayer::NewScene()
    {
        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OpenScene()
    {
        m_CurrentScenePath = FileDialog::OpenFile("Povox Scene (*.povox)\0*.povox\0");
        if (!m_CurrentScenePath.empty())
        {
            m_ActiveScene = CreateRef<Scene>();
            m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_SceneHierarchyPanel.SetContext(m_ActiveScene);

            SceneSerializer serializer(m_ActiveScene);
            serializer.Deserialize(m_CurrentScenePath);
        }
    }

    void EditorLayer::SaveScene()
    {
        if (!m_CurrentScenePath.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.Serialize(m_CurrentScenePath);
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        m_CurrentScenePath = FileDialog::SaveFile("Povox Scene (*.povox)\0*.povox\0");
        if (!m_CurrentScenePath.empty())
        {
            SaveScene();
        }
    }

    void EditorLayer::CloseApp()
    {
        Application::Get()->Close();
    }
}
