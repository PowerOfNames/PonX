#pragma once
#include "Povox/Renderer/OrthographicCamera.h"

#include "Povox/Events/Event.h"

#include "Povox/Events/MouseEvent.h"
#include "Povox/Events/ApplicationEvent.h"

namespace Povox {

	struct OrthographicCameraBounds
	{
		float Left, Right;
		float Bottom, Top;

		float GetWidth() { return Right - Left; }
		float GetHight() { return Top - Bottom; }
	};

	class OrthographicCameraController
	{
	public:
		OrthographicCameraController(float aspectRatio, bool rotation = false);
		~OrthographicCameraController() = default;


		void OnUpdate(float deltatime);
		void OnEvent(Event& e);

		void OnResize(float width, float height);

		OrthographicCamera& GetCamera() { return m_Camera; }
		const OrthographicCamera& GetCamera() const { return m_Camera; }


		float GetZoomLevel() { return m_ZoomLevel; }
		void SetZoomLevel(float zoomlevel) { m_ZoomLevel = zoomlevel; }


		const OrthographicCameraBounds& GetBounds() const { return m_Bounds; }	
	
	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);

	private:
		float m_AspectRatio;
		float m_ZoomLevel = 1.0f;
		OrthographicCameraBounds m_Bounds;
		OrthographicCamera m_Camera;

		bool m_Rotation;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		float m_CameraRotation = 0.0f; // in degrees anti clockwise direction
		
		float m_CameraControllSpeed = 3.0f;
		float m_CameraRotation_Speed = 180.0f;
	};

}
