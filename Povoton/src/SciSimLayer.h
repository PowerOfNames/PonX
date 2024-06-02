#pragma once
#include <Povox.h>

#include "Panels/SciParticleInformationPanel.h"
#include "Particles/SciParticleRenderer.h"


namespace Povox {

	class SciSimLayer : public Povox::Layer
	{
	public:
		SciSimLayer();
		~SciSimLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Povox::Timestep deltatime) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Povox::Event& e) override;
	private:
		//void ManageEntitySelection();

		bool OnFramebufferResize(Povox::FramebufferResizeEvent& e);

		bool OnKeyPressed(Povox::KeyPressedEvent& e);
		bool OnMouseButtonPressed(Povox::MouseButtonPressedEvent& e);

		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		void CloseApp();
	private:
		//Povox::EditorCamera m_EditorCamera;
		Povox::PerspectiveCameraController m_PerspectiveController;
		
		Povox::Ref<SciParticleRenderer> m_SciRenderer = nullptr;

		Povox::Ref<SciParticleSet> m_ActiveParticleSet = nullptr;

		TimerStatistics m_TimerStatistics;
		float m_Deltatime = 0.0f;

		uint32_t m_MaxParticleDraws = 0;

		bool m_GUICollapsed = false;
		bool m_ViewportIsFocused = false, m_ViewportIsHovered = false;
		glm::vec2 m_WindowSize = { 0.0f, 0.0f };
		bool m_WindowResized, m_ViewportResized = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2];

		bool m_PrimaryCamera = true;

		bool m_DemoActive = false;

	// Panels
		SciParticleInformationPanel m_ParticleInformationPanel;
		int m_GizmoType = -1;
		bool m_GizmoSnap = false;
	};

}
