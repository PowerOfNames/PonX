#include "pxpch.h"
#include "Povox/Renderer/PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>


namespace Povox {

	PerspectiveCamera::PerspectiveCamera(float aspectRatio)
		: m_AspectRatio(aspectRatio), m_ViewMatrix(1.0f)
	{
		SetProjectionMatrix(aspectRatio);
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


		glm::mat4 view = glm::lookAt(m_Position, m_Position + m_CameraFront, m_CameraUp);
		m_ViewMatrix = view;
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

}