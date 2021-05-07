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
	private:
		OrthographicCameraController m_CameraController;

		Ref<Texture2D> m_TextureLogo;
		Ref<SubTexture2D> m_SubTextureLogo;
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		std::string m_CurrentScenePath;
		Entity m_SquareEntity;
		Entity m_CameraEntity;
		Entity m_SecondCamera;

		float m_Deltatime = 0.0f;

		bool m_ViewportIsFocused = false, m_ViewportIsHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };

		Ref<Shader> m_FlatColorShader;
		glm::vec4 m_SquareColor1 = { 32.0f / 255, 95.0f / 255, 83.0f / 255 , 1.0f };
		glm::vec2 m_SquarePos = { 0.0f, 0.0f };
		glm::vec2 m_RotationVel = { 0.05f, 0.05f };

		bool m_PrimaryCamera = true;

		bool m_DemoActive = false;

	// Panels
		SceneHierarchyPanel m_SceneHierarchyPanel;
	};

}