#pragma once
#include "Povox/Renderer/PerspectiveCamera.h"

#include "Povox/Events/Event.h"
#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Events/MouseEvent.h"

namespace Povox {

	class PerspectiveCameraController
	{
	public:
		PerspectiveCameraController(float width, float height);
		~PerspectiveCameraController() = default;

		void OnUpdate(float deltaTime);
		void OnEvent(Event& event);

		void SetTranslationSpeed(float speed);
		void SetRotationSpeed(float speed);

		inline PerspectiveCamera& GetCamera() { return m_Camera; }
		inline const PerspectiveCamera& GetCamera() const { return m_Camera; }

	private:
		bool OnWindowResized(WindowResizeEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);

	private:
		float m_Width, m_Height;
		float m_AspectRatio;
		float m_ZoomLevel;
		PerspectiveCamera m_Camera;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraFront = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_CameraUp = { 0.0f, 1.0f, 0.0f };

		float m_Pitch = 0.0f, m_Yaw = -90.0f, m_Roll = 0.0f;
		float m_MousePosX , m_MousePosY;

		float m_FOV = 45.0f; // with 1.0f being minimum and 90.0f being max

		float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 1.0f;
	};

}