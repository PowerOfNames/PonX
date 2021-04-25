#pragma once
#include <Povox.h>

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
		Povox::OrthographicCameraController m_CameraController;

	private:
		Ref<Texture2D> m_TextureLogo;
		Ref<SubTexture2D> m_SubTextureLogo;
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Entity m_SquareEntity;
		Entity m_CameraEntity;

		float m_Deltatime = 0.0f;

		bool m_ViewportIsFocused = false, m_ViewportIsHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };

		Ref<Shader> m_FlatColorShader;
		glm::vec4 m_SquareColor1 = { 32.0f / 255, 95.0f / 255, 83.0f / 255 , 1.0f };
		glm::vec2 m_SquarePos = { 0.0f, 0.0f };


		glm::vec2 m_RotationVel = { 0.05f, 0.05f };
	};

}