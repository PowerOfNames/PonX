#pragma once

#include <glm/glm.hpp>


namespace Povox {

	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(float FOV, float aspectRatio, float nearClip, float farClip);
		~PerspectiveCamera() = default;

		inline const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position);

		inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		void RecalculateViewMatrix();

		inline const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		void SetProjectionMatrix(float FOV, float aspectRatio, float nearClip, float farClip);

		inline const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

		void SetForward(glm::vec3 front);
		inline const glm::vec3& GetForward() const { return m_CameraFront; }

		const std::array<glm::vec4, 8>& GetFrustumCorners() const { return m_FrustumCorners; }

		inline float GetFOV() const { return m_FOV; }
		inline float GetAspectRatio() const { return m_AspectRatio; }
		inline float GetNearClip() const { return m_NearClip; }
		inline float GetFarClip() const { return m_FarClip; }

	private:
		void CalculateFrustumCorners();
	private:
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraFront = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_CameraUp = { 0.0f, 1.0f, 0.0f };

		float m_AspectRatio;
		float m_FOV;
		float m_NearClip;
		float m_FarClip;

		std::array<glm::vec4, 8> m_FrustumCorners;
	};

}