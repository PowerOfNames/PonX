#include "SciSimLayer.h"

#include <ImGui/imgui.h>
#include <ImGuizmo.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Povox/Utils/PlatformsUtils.h"
#include "Povox/Math/Math.h"

namespace Povox {

	SciSimLayer::SciSimLayer()
        : Layer("Povoton"), m_PerspectiveController(1600, 900)
    {
    }

    void SciSimLayer::OnAttach()
    {
        PX_PROFILE_FUNCTION();


		m_WindowSize = m_ViewportSize = { Povox::Application::Get()->GetWindow().GetWidth(), Povox::Application::Get()->GetWindow().GetHeight() };


        //m_EditorCamera = Povox::EditorCamera(90.0f, (m_ViewportSize.x / m_ViewportSize.y * 1.0f), 0.1f, 1000.0f);
		PX_INFO("Camera Aspect Ratio: {}", (m_ViewportSize.x / m_ViewportSize.y * 1.0f));
		
		// Generate set or load from disk here, then pass data for simulation (continuous calculations) into SciParticleRenderer 
		// or just for visualization (one time compute for geometry calculation)
		Povox::BufferLayout particleLayout = BufferLayout({
			{ Povox::ShaderDataType::Float4, "PositionRadius" },
			{ Povox::ShaderDataType::Float4, "Velocity" },
			{ Povox::ShaderDataType::Float4, "Color" }
			});
		{
			SciParticleSetSpecification specs{};
			specs.ParticleLayout = particleLayout;
			specs.RandomGeneration = true;
			specs.GPUSimulationActive = true;
			specs.DebugName = "ParticleTestSet";
			m_ActiveParticleSet = Povox::CreateRef<SciParticleSet>(specs);
		}
		m_ParticleInformationPanel.SetContext(m_ActiveParticleSet);		

		{
			SciParticleRendererSpecification specs{};
			specs.ViewportWidth = m_ViewportSize.x;
			specs.ViewportHeight = m_ViewportSize.y;
			specs.ParticleLayout = particleLayout;
			m_SciRenderer = Povox::CreateRef<SciParticleRenderer>(specs);

			m_SciRenderer->Init();

			m_SciRenderer->LoadParticleSet("TestSet", m_ActiveParticleSet);
		}
    }


    void SciSimLayer::OnDetach()
    {
        PX_PROFILE_FUNCTION();

		m_SciRenderer->Shutdown();
    }


    void SciSimLayer::OnUpdate(Povox::Timestep deltatime)
    {
        PX_PROFILE_FUNCTION();

		        
		m_TimerStatistics.AddTimestamp(deltatime*1000.0f);
		m_Deltatime = deltatime;

        // Resize
		if(m_ViewportResized)
        {
			PX_INFO("New ViewportWidth and Height: {} {}", m_ViewportSize.x, m_ViewportSize.y);
			PX_INFO("New WindowWidth and Height: {} {}", m_WindowSize.x, m_WindowSize.y);

			Povox::Renderer::WaitForDeviceFinished();
			Povox::Renderer::OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);

            //m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
			m_PerspectiveController.ResizeViewport(m_ViewportSize.x, m_ViewportSize.y);

            m_SciRenderer->OnResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_ViewportResized = false;
        }


        //m_EditorCamera.OnUpdate(deltatime);
		m_PerspectiveController.OnUpdate(deltatime);

		m_SciRenderer->ResetStatistics();
		
		// Update particles, if set this will simulate using compute shaders
		m_ActiveParticleSet->OnUpdate(deltatime);

		m_SciRenderer->Begin(m_PerspectiveController.GetCamera());
		m_SciRenderer->OnUpdate(deltatime, m_MaxParticleDraws);

		// Now we draw the particle sets result
		m_SciRenderer->DrawParticleSet(m_ActiveParticleSet, m_MaxParticleDraws);

		//m_SciRenderer->End();
		m_SciRenderer->DrawParticleSilhouette(m_MaxParticleDraws);

		//CopyFinalImage into current SwapchainImage
		{
			if (!Application::Get()->GetSpecification().ImGuiEnabled)
				Renderer::PrepareSwapchainImage(m_SciRenderer->GetFinalImage());
			else
				Renderer::CreateFinalImage(m_SciRenderer->GetFinalImage());
		}

		// TODO: Move to MousePicking system
		{
			auto [mx, my] = ImGui::GetMousePos();
			mx -= m_ViewportBounds[0].x;
			my -= m_ViewportBounds[0].y;
			glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
			my = viewportSize.y - my;
			int mouseX = (int)mx;
			int mouseY = (int)my;

			if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
			{
				//uint64_t pixelData = (uint64_t)m_ImGuiViewportFB->GetColorAttachment(1)->ReadPixel(mouseX, mouseY);
			}
		}
    }

	void SciSimLayer::OnImGuiRender()
	{
		PX_PROFILE_FUNCTION();

		//PX_TRACE("SciSimLayer::OnImguiRender Started!");
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
		//auto stats = m_ActiveScene->GetStats();



		if (!m_GUICollapsed)
		{
			ImGui::Begin(" Stats ", &m_GUICollapsed);
			// By default, if we don't enable ScrollX the sizing policy for each columns is "Stretch"
			// Each columns maintain a sizing weight, and they will occupy all available width.
			static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableColumnFlags_WidthStretch | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_BordersOuter;
			
			//ImGui::SameLine(); HelpMarker("Using the _Resizable flag automatically enables the _BordersInnerV flag as well, this is why the resize borders are still showing when unchecking this.");
		
			if (ImGui::BeginTable("table1", 2, flags))
			{		
				ImGui::TableHeadersRow();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("FPS:");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.2f", 1.0f / m_Deltatime);
				
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("DeltaTime dt:");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.2fms", m_Deltatime * 1000.0);
				
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("TotalAverage:");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.2fms", m_TimerStatistics.GetAverage());
				
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("TotalMax:"); 
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.2fms", m_TimerStatistics.GetMax());
				
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Average dt 1s:");
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.2fms", m_TimerStatistics.GetTimespanAverage());				

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Max dt 1s:"); 
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.2fms", m_TimerStatistics.GetTimespanMax());								
				
				ImGui::EndTable();				
			}
			if (ImGui::Button("Reset"))
				m_TimerStatistics.Reset();

			// 			ImGui::Text("DrawCalls: %d", stats.DrawCalls);
			// 			ImGui::Text("Quads: %d", stats.QuadCount);
			// 			ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
			// 			ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
			//ImGui::Text("Deltatime: %fms", m_Deltatime * 1000.0);
			//ImGui::Text("FPS: %f", 1.0f / m_Deltatime);
			ImGui::Separator();

			Povox::RendererStatistics rendererStats = Povox::Renderer::GetStatistics();
			for (const auto& [poolName, queries] : rendererStats.PipelineStats)
			{
				ImGui::Text(poolName.c_str());
				for(const auto& [query, stat] : queries)
				{
					std::string caption = query + " %llu";
					ImGui::Text(caption.c_str(), stat);
				}
				ImGui::Separator();
			}			
			ImGui::Separator();
			for (const auto& [name, value] : rendererStats.TimestampResults)
			{
				double timeInMS = value / 1000000.0;
				const std::string caption = name + ": %.6fms";
				ImGui::Text(caption.c_str(), timeInMS);
			}
			ImGui::Separator();
			ImGui::Text("TotalFrames: %u", rendererStats.State->TotalFrames);


			std::string name = "None";
			//if (m_HoveredEntity)
			//	name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
			ImGui::Text("Hovered Entity: %s", name.c_str());

			ImGui::End(); // Renderer Stats


			ImGui::Begin("Test Tab");
			ImGui::Checkbox("Demo ImGui", &m_DemoActive);
			if (m_DemoActive)
				ImGui::ShowDemoWindow();
			ImGui::End();

			ImGui::Begin("Editor Camera");
			glm::vec3 cameraPos = m_PerspectiveController.GetCamera().GetPosition();
			glm::vec3 front = m_PerspectiveController.GetCamera().GetForward();
			glm::mat4 view = glm::inverse(m_PerspectiveController.GetCamera().GetViewMatrix());
			glm::mat4 proj = m_PerspectiveController.GetCamera().GetProjectionMatrix();

			//glm::vec3 cameraPos = m_EditorCamera.GetPosition();
			ImGui::Text("Camera Position: {%f|%f|%f}", cameraPos.x, cameraPos.y, cameraPos.z);
			ImGui::Text("Camera Direction: {%f|%f|%f}", front.x, front.y, front.z);
			ImGui::Text("Camera ViewMatrix: {%f|%f|%f|%f}", view[0][0], view[1][0], view[2][0], view[3][0]);
			ImGui::Text("Camera ViewMatrix: {%f|%f|%f|%f}", view[0][1], view[1][1], view[2][1], view[3][1]);
			ImGui::Text("Camera ViewMatrix: {%f|%f|%f|%f}", view[0][2], view[1][2], view[2][2], view[3][2]);
			ImGui::Text("Camera ViewMatrix: {%f|%f|%f|%f}", view[0][3], view[1][3], view[2][3], view[3][3]);

			ImGui::Text("Camera ProjMatrix: {%f|%f|%f|%f}", proj[0][0], proj[1][0], proj[2][0], proj[3][0]);
			ImGui::Text("Camera ProjMatrix: {%f|%f|%f|%f}", proj[0][1], proj[1][1], proj[2][1], proj[3][1]);
			ImGui::Text("Camera ProjMatrix: {%f|%f|%f|%f}", proj[0][2], proj[1][2], proj[2][2], proj[3][2]);
			ImGui::Text("Camera ProjMatrix: {%f|%f|%f|%f}", proj[0][3], proj[1][3], proj[2][3], proj[3][3]);
			ImGui::End();


			// Panels
			m_ParticleInformationPanel.OnImGuiRender();

		} // Collapsed
		ImGui::End(); //Dockspace end

		// Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin(" Viewport ");

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewPortOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewPortOffset.x, viewportMinRegion.y + viewPortOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewPortOffset.x, viewportMaxRegion.y + viewPortOffset.y };

		m_ViewportIsFocused = ImGui::IsWindowFocused();
		m_ViewportIsHovered = ImGui::IsWindowHovered();
		Povox::Application::Get()->GetImGuiVulkanLayer()->BlockEvents(!m_ViewportIsFocused/* || !m_ViewportIsHovered*/);
		ImGui::GetWindowSize();

		ImGui::GetForegroundDrawList()->AddRect(
			ImVec2(viewportMinRegion.x + viewPortOffset.x, viewportMinRegion.y + viewPortOffset.y),
			ImVec2(viewportMaxRegion.x + viewPortOffset.x, viewportMaxRegion.y + viewPortOffset.y), IM_COL32(255, 0, 0, 255));

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		if (viewportPanelSize.x > 0.0f && viewportPanelSize.y > 0.0f
			&& (viewportPanelSize.x != m_ViewportSize.x || viewportPanelSize.y != m_ViewportSize.y))
		{
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			m_ViewportResized = true;
		}
		Povox::Ref<Povox::Image2D> final = Povox::Renderer::GetFinalImage(Povox::Renderer::GetLastFrameIndex());
		if (final)
		{
			ImTextureID texID = Povox::Renderer::GetGUIDescriptorSet(final);
			ImGui::Image(texID, ImVec2(m_ViewportSize.x, m_ViewportSize.y));
		}

		// Gizmos
		//ManageEntitySelection();

		//Viewport
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("ParticleRenderingControl");
		ImGui::DragInt("MaxParticleDraws", (int*)&m_MaxParticleDraws, 1, 0, 1024*1024);
		ImGui::End();
		
    }
// /*
// 	void SciSimLayer::ManageEntitySelection()
// 	{
// 		Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
// 		if (selectedEntity && m_GizmoType != -1)
// 		{
// 			ImGuizmo::SetOrthographic(false);
// 			ImGuizmo::SetDrawlist();
// 			ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);
// 
// 			// Runtime camera
// 			//auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
// 			//
// 			//const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
// 			//const glm::mat4& cameraProjection = camera.GetProjection();
// 			//glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());
// 
// 			// Editor Camera            
// 			const glm::mat4& cameraProjection = m_EditorCamera.GetProjection();
// 			glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();
// 
// 			// Entity Transform
// 			auto& tc = selectedEntity.GetComponent<TransformComponent>();
// 			glm::mat4 transform = tc.GetTransform();
// 
// 			m_GizmoSnap = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
// 			float snapValue = 0.5f;
// 			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
// 				snapValue = 45.0f;
// 
// 			float snapValues[] = { snapValue, snapValue, snapValue };
// 
// 			ImGui::Begin("Gizmos");
// 			ImGui::SameLine();
// 			switch ((ImGuizmo::OPERATION)m_GizmoType)
// 			{
// 				case ImGuizmo::TRANSLATE:
// 					ImGui::InputFloat3("Snap", &snapValues[0]);
// 					break;
// 				case ImGuizmo::ROTATE:
// 					ImGui::InputFloat3("Angle Snap", &snapValues[1]);
// 					break;
// 				case ImGuizmo::SCALE:
// 					ImGui::InputFloat3("Scale Snap", &snapValues[2]);
// 					break;
// 			}
// 
// 			ImGui::End();
// 			ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
// 				(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, m_GizmoSnap ? snapValues : nullptr);
// 
// 			if (ImGuizmo::IsUsing())
// 			{
// 				glm::vec3 translation, rotation, scale;
// 				Math::DecomposeTransform(transform, translation, rotation, scale);
// 				glm::vec3 deltaRotation = rotation - tc.Rotation;
// 				tc.Translation = translation;
// 				tc.Rotation += deltaRotation;
// 				tc.Scale = scale;
// 			}
// 		}
// */



    void SciSimLayer::OnEvent(Povox::Event& e)
    {
        //m_EditorCamera.OnEvent(e);
		m_PerspectiveController.OnEvent(e);

		Povox::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Povox::KeyPressedEvent>(PX_BIND_EVENT_FN(SciSimLayer::OnKeyPressed));
        dispatcher.Dispatch<Povox::MouseButtonPressedEvent>(PX_BIND_EVENT_FN(SciSimLayer::OnMouseButtonPressed));

		dispatcher.Dispatch<Povox::FramebufferResizeEvent>(PX_BIND_EVENT_FN(SciSimLayer::OnFramebufferResize));
    }
	bool SciSimLayer::OnFramebufferResize(Povox::FramebufferResizeEvent& e)
	{
		m_WindowResized = true;
		m_ViewportResized = true;

		m_WindowSize = { e.GetWidth(), e.GetHeight() };
		if (!Povox::Application::Get()->GetSpecification().ImGuiEnabled)
		{
			m_ViewportSize = m_WindowSize;
			m_ViewportResized = true;
		}

		return false;
	}
    bool SciSimLayer::OnKeyPressed(Povox::KeyPressedEvent& e)
    {
        //shortcuts
        if (e.GetRepeatCount() > 0)
            return false;
        bool controlPressed = Povox::Input::IsKeyPressed(Povox::Key::LeftControl) || Povox::Input::IsKeyPressed(Povox::Key::RightControl);
        bool shiftPressed = Povox::Input::IsKeyPressed(Povox::Key::LeftShift) || Povox::Input::IsKeyPressed(Povox::Key::RightShift);
        switch (e.GetKeyCode())
        {
            case Povox::Key::N:
            {
                if (controlPressed)
                {
                    NewScene();
                    break;
                }
            }
            case Povox::Key::O:
            {
                if (controlPressed)
                {
                    OpenScene();
                    break;
                }
            }
            case Povox::Key::S:
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
            case Povox::Key::Q:
            {
                m_GizmoType = -1;
                break;
            }
            case Povox::Key::W:
            {
                m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Povox::Key::E:
            {
                m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Povox::Key::R:
            {
                m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            }
			case Povox::Key::C:
			{			
				if (Povox::Application::Get()->GetSpecification().ImGuiEnabled)
				{
					m_GUICollapsed = !m_GUICollapsed;
				}				
				break;
			}
            case Povox::Key::Escape:
            {
                if (controlPressed)
                {
                    CloseApp();
                    break;
                }
				else
					ImGui::SetWindowFocus();
            }
        }
    }
    bool SciSimLayer::OnMouseButtonPressed(Povox::MouseButtonPressedEvent& e)
    {
        bool isAltPressed = Povox::Input::IsKeyPressed(Povox::Key::LeftAlt) || Povox::Input::IsKeyPressed(Povox::Key::RightAlt);
        switch (e.GetMouseButton())
        {
            case Povox::Mouse::ButtonLeft:
            {
                if(!isAltPressed && !ImGuizmo::IsOver() && m_ViewportIsHovered)
                    //m_ParticleInformationPanel.SetSelectedParticle(m_HoveredParticle);
                break;
            }
        }

        return false;
    }



	void SciSimLayer::NewScene()
    {
        //m_ActiveScene = Povox::CreateRef<Scene>(m_ViewportSize.x, m_ViewportSize.y);
        //m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_ParticleInformationPanel.SetContext(m_ActiveParticleSet);
    }

    void SciSimLayer::OpenScene()
    {
        //m_CurrentScenePath = FileDialog::OpenFile("Povox Scene (*.povox)\0*.povox\0");
//         if (!m_CurrentScenePath.empty())
//         {
//             //m_ActiveScene = CreateRef<Scene>(m_ViewportSize.x, m_ViewportSize.y);
//             //m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
// 			m_ParticleInformationPanel.SetContext(m_ActiveParticleSet);
// 
// 
// //             SceneSerializer serializer(m_ActiveScene);
// //             serializer.Deserialize(m_CurrentScenePath);
//         }
    }

    void SciSimLayer::SaveScene()
    {
//         if (!m_CurrentScenePath.empty())
//         {
//             SceneSerializer serializer(m_ActiveScene);
//             serializer.Serialize(m_CurrentScenePath);
//         }
//         else
//         {
//             SaveSceneAs();
//         }
    }

    void SciSimLayer::SaveSceneAs()
    {
        //m_CurrentScenePath = FileDialog::SaveFile("Povox Scene (*.povox)\0*.povox\0");
//         if (!m_CurrentScenePath.empty())
//         {
//             SaveScene();
//         }
    }

    void SciSimLayer::CloseApp()
    {
        Povox::Application::Get()->Close();
    }
}
