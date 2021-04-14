#include "pxpch.h"
#include "Povox/Renderer/OrthographicCameraController.h"

#include "Povox/Core/Input.h"
#include "Povox/Core/KeyCodes.h"


namespace Povox {

	OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
		: m_AspectRatio(aspectRatio), m_Bounds({ -aspectRatio * m_ZoomLevel, aspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel }), m_Camera(m_Bounds.Left, m_Bounds.Right, m_Bounds.Bottom, m_Bounds.Top), m_Rotation(rotation)
	{
	
	}

	void OrthographicCameraController::OnUpdate(float deltatime)
	{
		PX_PROFILE_FUNCTION();
		
	// Left
		if (Input::IsKeyPressed(Key::A))
		{
			m_CameraPosition.x -= cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y -= sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}
	// Right
		else if (Input::IsKeyPressed(Key::D))
		{
			m_CameraPosition.x += cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y += sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}
	// Up
		if (Input::IsKeyPressed(Key::W))
		{
			m_CameraPosition.x += -sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y +=  cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}
	// Down
		else if (Input::IsKeyPressed(Key::S))
		{
			m_CameraPosition.x -= -sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y -= cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}

		if (m_Rotation)
		{
		// Tilting left
			if (Input::IsKeyPressed(Key::Q))
			{
				m_CameraRotation += m_CameraRotation_Speed * deltatime;
			}
		// Tilting right
			else if (Input::IsKeyPressed(Key::E))
			{
				m_CameraRotation -= m_CameraRotation_Speed * deltatime;
			}

			if (m_CameraRotation > 180.0f)
				m_CameraRotation -= 360.0f;
			else if (m_CameraRotation <= -180.0f)
				m_CameraRotation += 360.0f;
			m_Camera.SetRotation(m_CameraRotation);
		}
		m_Camera.SetPosition(m_CameraPosition);
	}

	void OrthographicCameraController::OnEvent(Event& e)
	{
		PX_PROFILE_FUNCTION();


		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(PX_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(OrthographicCameraController::OnWindowResized));
	}

	void OrthographicCameraController::OnResize(float width, float height)
	{
		m_AspectRatio = width / height;
		m_Bounds = { -m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel };
		m_Camera.SetProjectionMatrix(m_Bounds.Left, m_Bounds.Right, m_Bounds.Bottom, m_Bounds.Top);
	}

	bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		PX_PROFILE_FUNCTION();


		m_ZoomLevel -= e.GetYOffset() * 0.25f;
		m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		m_Bounds = { -m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel };
		m_Camera.SetProjectionMatrix(m_Bounds.Left, m_Bounds.Right, m_Bounds.Bottom, m_Bounds.Top);

		return false;
	}

	bool OrthographicCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();


		m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
		m_Bounds = { -m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel };
		m_Camera.SetProjectionMatrix(m_Bounds.Left, m_Bounds.Right, m_Bounds.Bottom, m_Bounds.Top);

		return false;
	}

}