#include "pxpch.h"
#include "Povox/Renderer/OrthographicCameraController.h"

#include "Povox/Core/Input.h"
#include "Povox/Core/KeyCodes.h"


namespace Povox {

	OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
		: m_AspectRatio(aspectRatio), m_ZoomLevel(1.0f), m_Rotation(rotation), m_Camera(-aspectRatio * m_ZoomLevel, aspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel)
	{
	}

	void OrthographicCameraController::OnUpdate(float ts)
	{
	// 'W' 'A' 'S' 'D' for UP, Left, Down, Right movement with speed 'm_CamerControllSpeed'.
	// ---
	// 'Q' and 'E' tilt the camera to the left or the right

	// Up
		if (Input::IsKeyPressed(PX_KEY_W))
		{
			m_CameraPosition.x += -sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
			m_CameraPosition.y +=  cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
		}
	// Down
		else if (Input::IsKeyPressed(PX_KEY_S))
		{
			m_CameraPosition.x -= -sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
			m_CameraPosition.y -=  cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
		}
	// Left
		if (Input::IsKeyPressed(PX_KEY_A))
		{
			m_CameraPosition.x -= cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
			m_CameraPosition.y -= sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
		}
	// Right
		else if (Input::IsKeyPressed(PX_KEY_D))
		{
			m_CameraPosition.x += cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
			m_CameraPosition.y += sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * ts;
		}

		if (m_Rotation)
		{
		// Tilting left
			if (Input::IsKeyPressed(PX_KEY_Q))
			{
				m_CameraRotation += m_CameraRotation_Speed * ts;
			}
		// Tilting right
			else if (Input::IsKeyPressed(PX_KEY_E))
			{
				m_CameraRotation -= m_CameraRotation_Speed * ts;
			}

			if (m_CameraRotation < 0)
			{
				m_CameraRotation = 360;
			}
			else if (m_CameraRotation > 360)
			{
				m_CameraRotation = 0;
			}
			m_Camera.SetRotation(m_CameraRotation);
		}
		m_Camera.SetPosition(m_CameraPosition);
	}

	void OrthographicCameraController::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(PX_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
		dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(OrthographicCameraController::OnWindowResized));
	}

	bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		m_ZoomLevel -= e.GetYOffset() * 0.25f;
		m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
		m_Camera.SetProjectionMatrix(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);

		return false;
	}

	bool OrthographicCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		m_AspectRatio = (float)e.GetHeight() / (float)e.GetWidth();
		m_Camera.SetProjectionMatrix(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);

		return false;
	}

}