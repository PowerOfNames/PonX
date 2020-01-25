#pragma once
#include "Povox/Renderer/OrthographicCamera.h"

#include "Povox/Events/Event.h"

#include "Povox/Events/MouseEvent.h"
#include "Povox/Events/ApplicationEvent.h"

namespace Povox {

	class OrthographicCameraController
	{
	public:
		OrthographicCameraController(float aspectRatio, bool rotation = 0);
		~OrthographicCameraController() = default;

		void OnUpdate(float ts);
		void OnEvent(Event& e);

		inline OrthographicCamera& GetCamera() { return m_Camera; }
		inline const OrthographicCamera& GetCamera() const { return m_Camera; }

	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);

	private:
		float m_AspectRatio;
		float m_ZoomLevel;
		OrthographicCamera m_Camera;
		bool m_Rotation;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		float m_CameraRotation = 0.0f; // in degrees anti clockwise direction
		
		float m_CameraControllSpeed = 0.1f;
		float m_CameraRotation_Speed = 180.0f;
	};

}
