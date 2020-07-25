#pragma once

#include <glm/glm.hpp>


namespace Povox {

	class PerspectiveCamera
	{
	public:
		PerspectiveCamera(float width, float height);
		~PerspectiveCamera() = default;


		inline const glm::vec3& GetPosition() { return m_Position; }
		void SetPosition(const glm::vec3& position);
	
		inline const glm::mat4& GetView() { return m_ViewMatrix; }
		void RecalculateViewMatrix();

		inline const glm::mat4& GetProjection() { return m_ProjectionMatrix; }
		void SetProjectionMatrix(float aspectRatio, float FOV = 45.0f);

		inline const glm::mat4& GetViewProjection() { return m_ViewProjectionMatrix; }

		inline const glm::vec3& GetFront() const { return m_CameraFront; }
		void SetCameraFront(glm::vec3 front);

		inline const glm::vec3& GetUp() const { return m_CameraUp; }
		inline const glm::vec3& GetRight() const { return m_CameraRight; }
		void RecalculateUpandRight();

		inline const float GetAspectRatio() const { return m_AspectRatio; }

		inline const float GetWidth() const { return m_Width; }
		void SetWidth(float width);
		inline const float GetHeight() const { return m_Height; }
		void SetHeight(float height);

	private:
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position	= { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraFront = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_Up			= { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_CameraUp	= { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_CameraRight = {1.0f, 0.0f, 0.0f};

		float m_AspectRatio, m_Width, m_Height;
	};

}