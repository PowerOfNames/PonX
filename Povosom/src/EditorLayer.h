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
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		void CloseApp();
	private:
		EditorCamera m_EditorCamera;
		OrthographicCameraController m_OrthoCamControl;

		std::vector<Renderable> m_DrawList;

		Ref<Framebuffer> m_ImGuiViewportFB;
		Ref<RenderPass> m_ImGuiRenderpass;
		Ref<Pipeline> m_FlatColorPipeline;
		Ref<Pipeline> m_TexturePipeline;

		Ref<Texture2D> m_LogoTexture;
		
		
		Ref<Scene> m_ActiveScene;
		std::string m_CurrentScenePath;

		Entity m_HoveredEntity;

		float m_Deltatime = 0.0f;

		bool m_ViewportIsFocused = false, m_ViewportIsHovered = false;
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
