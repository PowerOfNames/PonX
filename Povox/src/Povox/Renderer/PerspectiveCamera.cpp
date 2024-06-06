#include "pxpch.h"
#include "Povox/Renderer/PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>


namespace Povox {

	PerspectiveCamera::PerspectiveCamera(float FOV, float aspectRatio, float nearClip, float farClip)
		: m_FOV(FOV), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), m_ViewMatrix(1.0f)
	{
		SetProjectionMatrix(FOV, aspectRatio, nearClip, farClip);
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

	void PerspectiveCamera::SetProjectionMatrix(float FOV, float aspectRatio, float nearClip, float farClip)
	{
		PX_PROFILE_FUNCTION();

		m_FOV = FOV;
		m_AspectRatio = aspectRatio;
		m_NearClip = nearClip;
		m_FarClip = farClip;

		m_ProjectionMatrix = glm::perspective(glm::radians(FOV), aspectRatio, nearClip, farClip);
		CalculateFrustumCorners();
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::SetForward(glm::vec3 front)
	{
		m_CameraFront = front;
		RecalculateViewMatrix();
	}

	/**
	 * @brief 
	 * Calculates the frustum corners in view space of the camera, starting with bottom right near(0)/far(1), clockwise.
	 * 
	 */
	void PerspectiveCamera::CalculateFrustumCorners()
	{
		float tanHalfFOV = glm::tan(glm::radians(m_FOV) / 2.0f);
		float nearHeight = 2.0f * tanHalfFOV * m_NearClip;
		float nearWidth = nearHeight * m_AspectRatio;
		float farHeight = 2.0f * tanHalfFOV * m_FarClip;
		float farWidth = farHeight * m_AspectRatio;

		m_FrustumCorners[0] = glm::vec4(-nearWidth / 2.0f, nearHeight / 2.0f, -m_NearClip, 1.0f);	//near topLeft
		m_FrustumCorners[1] = glm::vec4(-farWidth / 2.0f, farHeight / 2.0f, -m_FarClip, 1.0f);		//far topLeft
		m_FrustumCorners[2] = glm::vec4(nearWidth / 2.0f, nearHeight / 2.0f, -m_NearClip, 1.0f);	//near topRight
		m_FrustumCorners[3] = glm::vec4(farWidth / 2.0f, farHeight / 2.0f, -m_FarClip, 1.0f);		//far topRight
		m_FrustumCorners[4] = glm::vec4(nearWidth / 2.0f, -nearHeight / 2.0f, -m_NearClip, 1.0f);	//near bottomRight
		m_FrustumCorners[5] = glm::vec4(farWidth / 2.0f, -farHeight / 2.0f, -m_FarClip, 1.0f);		//far bottomRight
		m_FrustumCorners[6] = glm::vec4(-nearWidth / 2.0f, -nearHeight / 2.0f, -m_NearClip, 1.0f);	//near bottomLeft
		m_FrustumCorners[7] = glm::vec4(-farWidth / 2.0f, -farHeight / 2.0f, -m_FarClip, 1.0f);		//far bottomLeft
	}

}