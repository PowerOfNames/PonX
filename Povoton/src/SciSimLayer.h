#pragma once
#include <Povox.h>

#include "Panels/SceneHierarchyPanel.h"


namespace Povox {

	class SciSimLayer : public Layer
	{
	public:
		SciSimLayer();
		~SciSimLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep deltatime) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		void ManageEntitySelection();

		bool OnFramebufferResize(FramebufferResizeEvent& e);

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		void CloseApp();
	private:
		EditorCamera m_EditorCamera;
		OrthographicCameraController m_OrthoCamControl;
		
		Ref<Scene> m_ActiveScene;
		std::string m_CurrentScenePath;

		Entity m_HoveredEntity;

		float m_Deltatime = 0.0f;

		bool m_GUICollapsed = false;
		bool m_ViewportIsFocused = false, m_ViewportIsHovered = false;
		glm::vec2 m_WindowSize = { 0.0f, 0.0f };
		bool m_WindowResized, m_ViewportResized = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		bool m_PrimaryCamera = true;

		bool m_DemoActive = false;

	// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		int m_GizmoType = -1;
		bool m_GizmoSnap = false;

	};

}
