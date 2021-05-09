#pragma once
#include <Povox.h>

#include "Panels/SceneHierarchyPanel.h"


namespace Povox {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep deltatime) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		void CloseApp();
	private:
		EditorCamera m_EditorCamera;
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		std::string m_CurrentScenePath;

		float m_Deltatime = 0.0f;

		bool m_ViewportIsFocused = false, m_ViewportIsHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };

		bool m_PrimaryCamera = true;

		bool m_DemoActive = false;

	// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
		int m_GizmoType = -1;
		bool m_GizmoSnap = false;

	};

}