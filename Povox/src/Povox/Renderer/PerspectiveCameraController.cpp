#include "pxpch.h"
#include "Povox/Renderer/PerspectiveCameraController.h"

#include "Povox/Core/Input.h"
#include "Povox/Core/KeyCodes.h"

namespace Povox {



	PerspectiveCameraController::PerspectiveCameraController(float FOV, float width, float height, float nearClip, float farClip, const std::string name/* = "CameraDebugName"*/)
		: m_Width(width), m_Height(height), m_AspectRatio(width / height), m_NearClip(nearClip), m_FarClip(farClip), m_ZoomLevel(1.0f), m_Camera(FOV, width / height, nearClip, farClip), m_DebugName(name)
	{
		PX_PROFILE_FUNCTION();

		m_MousePosX = width / 2;
		m_MousePosY = height / 2;
	}

	void PerspectiveCameraController::OnUpdate(Timestep deltaTime)
	{
		PX_PROFILE_FUNCTION();

		if (Input::IsKeyPressed(Key::W))
		{
			m_CameraPosition += m_CameraTranslationSpeed * deltaTime * m_CameraFront;
		}
		else if (Input::IsKeyPressed(Key::S))
		{
			m_CameraPosition -= m_CameraTranslationSpeed * deltaTime * m_CameraFront;
		}

		if (Input::IsKeyPressed(Key::D))
		{
			m_CameraPosition += m_CameraTranslationSpeed * deltaTime * glm::normalize(glm::cross(m_CameraFront, m_CameraUp));
		}
		else if (Input::IsKeyPressed(Key::A))
		{
			m_CameraPosition -= m_CameraTranslationSpeed * deltaTime * glm::normalize(glm::cross(m_CameraFront, m_CameraUp));
		}

		m_Camera.SetPosition(m_CameraPosition);
	}

	void PerspectiveCameraController::OnEvent(Event& event)
	{
		PX_PROFILE_FUNCTION();

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(PerspectiveCameraController::OnWindowResized));		
		dispatcher.Dispatch<MouseMovedEvent>(PX_BIND_EVENT_FN(PerspectiveCameraController::OnMouseMoved));
		dispatcher.Dispatch<MouseScrolledEvent>(PX_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
	}

	void PerspectiveCameraController::SetTranslationSpeed(float speed)
	{
		PX_PROFILE_FUNCTION();

		m_CameraTranslationSpeed = speed;
	}

	void PerspectiveCameraController::SetRotationSpeed(float speed)
	{
		PX_PROFILE_FUNCTION();

		m_CameraRotationSpeed = speed;
	}

	void PerspectiveCameraController::ResizeViewport(float width, float height)
	{
		float aspectRatio = width / height;
		m_Width = width;
		m_Height = height;
		m_Camera.SetProjectionMatrix(m_FOV, aspectRatio, m_NearClip, m_FarClip);
	}

	bool PerspectiveCameraController::OnWindowResized(WindowResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();
				
		ResizeViewport((float)e.GetWidth(), (float)e.GetHeight());

		return false;
	}

	bool PerspectiveCameraController::OnMouseMoved(MouseMovedEvent& e)
	{
		PX_PROFILE_FUNCTION();


		if (!m_MouseCaught)
		{
			m_MousePosX = e.GetX();
			m_MousePosY = e.GetY();
			m_MouseCaught = true;
		}

		float xOffset = e.GetX() - m_MousePosX;
		float yOffset = m_MousePosY - e.GetY();
		m_MousePosX	= e.GetX();
		m_MousePosY	= e.GetY();

		

		float sensitivity = 0.2f;
		xOffset	*= sensitivity;
		yOffset *= sensitivity;

		m_Yaw	+= xOffset;
		m_Pitch += yOffset;


		if (m_Pitch > 89)
			m_Pitch = 89;
		if (m_Pitch < -89)
			m_Pitch = -89;

		if (m_Yaw >= 360)
			m_Yaw = 0;
		if (m_Yaw <= -360)
			m_Yaw = 0;
		glm::vec3 direction;
		direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		direction.y = sin(glm::radians(m_Pitch));
		direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

		m_CameraFront = glm::normalize(direction);
		m_Camera.SetForward(m_CameraFront);
		return false;
	}

	bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e)
	{
		if (m_FOV > 1.0f && m_FOV < 90.0f)
		{
			m_FOV -= e.GetYOffset();
		}
		else if (m_FOV < 1.0f)
		{
			m_FOV = 1.0f;
		}
		else if (m_FOV > 90.0f)
		{
			m_FOV = 90.0f;
		}
		m_Camera.SetProjectionMatrix(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);

		return false;
	}

}