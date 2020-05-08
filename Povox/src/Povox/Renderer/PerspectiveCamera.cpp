#include "pxpch.h"
#include "Povox/Renderer/PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>


namespace Povox {

	PerspectiveCamera::PerspectiveCamera(float width, float height)
		: m_Width(width), m_Height(height), m_ViewMatrix(1.0f)
	{
		m_AspectRatio = width / height;
		SetProjectionMatrix(m_AspectRatio);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::SetPosition(const glm::vec3& position)
	{
		PX_PROFILE_FUNCTION();


		m_Position = position;
		RecalculateViewMatrix();	
	}

	void PerspectiveCamera::RecalculateViewMatrix()
	{
		PX_PROFILE_FUNCTION();


		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_CameraFront, m_CameraUp);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::SetProjectionMatrix(float aspectRatio, float FOV)
	{
		PX_PROFILE_FUNCTION();

		m_AspectRatio = aspectRatio;
		m_ProjectionMatrix = glm::perspective(glm::radians(FOV), aspectRatio, 0.1f, 100.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::SetCameraFront(glm::vec3 front)
	{
		m_CameraFront = front;
		RecalculateViewMatrix();
	}

	void PerspectiveCamera::SetWidth(float width)
	{
		m_Width = width;
		m_AspectRatio = m_Width / m_Height;
	}

	void PerspectiveCamera::SetHeight(float height)
	{
		m_Height = height;
		m_AspectRatio = m_Width / m_Height;
	}

}