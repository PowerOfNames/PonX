#include "pxpch.h"
#include "Povox/Renderer/OrthographicCameraController.h"

#include "Povox/Core/Input.h"
#include "Povox/Core/KeyCodes.h"


namespace Povox {

	OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
		: m_AspectRatio(aspectRatio), m_ZoomLevel(1.0f), m_Rotation(rotation), m_Camera(-aspectRatio * m_ZoomLevel, aspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel)
	{
		Input::Remap("camera_move_up", KeyAlternative(Keys::W));
		Input::Remap("camera_move_left", KeyAlternative(Keys::A));
		Input::Remap("camera_move_down", KeyAlternative(Keys::S));
		Input::Remap("camera_move_right", KeyAlternative(Keys::D));

		Input::Remap("camera_rotate_clockwise", KeyAlternative(Keys::E));
		Input::Remap("camera_rotate_anticlockwise", KeyAlternative(Keys::Q));
	}

	void OrthographicCameraController::OnUpdate(float deltatime)
	{
	// 'W' 'A' 'S' 'D' for UP, Left, Down, Right movement with speed 'm_CameraControllSpeed'.
	// ---
	// 'Q' and 'E' tilt the camera to the left or the right
		
	// Left
		if (Input::IsInputPressed("camera_move_left"))
		{
			m_CameraPosition.x -= cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y -= sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}
	// Right
		else if (Input::IsInputPressed("camera_move_right"))
		{
			m_CameraPosition.x += cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y += sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}
	// Up
		if (Input::IsInputPressed("camera_move_up"))
		{
			m_CameraPosition.x += -sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y +=  cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}
	// Down
		else if (Input::IsInputPressed("camera_move_down"))
		{
			m_CameraPosition.x -= -sin(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
			m_CameraPosition.y -= cos(glm::radians(m_CameraRotation)) * m_CameraControllSpeed * deltatime;
		}

		if (m_Rotation)
		{
		// Tilting left
			if (Input::IsInputPressed("camera_rotate_anticlockwise"))
			{
				m_CameraRotation += m_CameraRotation_Speed * deltatime;
			}
		// Tilting right
			else if (Input::IsInputPressed("camera_rotate_clockwise"))
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
		m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
		m_Camera.SetProjectionMatrix(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);

		return false;
	}

}