#pragma once

#include <glm/glm.hpp>


namespace Povox {

	class Light
	{
	protected:
		Light(const std::string& name);
		virtual ~Light() = default;
	public:
		virtual const glm::vec3& GetPosition() = 0;
		virtual void SetPosition(const glm::vec3& position) = 0;

		virtual const glm::vec3& const GetColor() = 0;
		virtual void SetColor(const glm::vec3& color) = 0;

		virtual float GetIntensity() = 0;
		virtual void SetIntensity(float intensity) = 0;

		virtual const std::string& GetName() = 0;


	protected:
		std::string m_Name;
	};

	class PointLight : public Light
	{
	public:
		PointLight(const std::string& name = "light", const glm::vec3& position = { 0.0f, 0.0f, 0.0f }, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }, float intensity = 1.0f);
		~PointLight() = default;

		virtual inline const glm::vec3& GetPosition() override { return m_Position; }
		virtual inline void SetPosition(const glm::vec3& position) override { m_Position = position; }

		virtual inline const glm::vec3& GetColor() override { return m_Color; }
		virtual inline void SetColor(const glm::vec3& color) override { m_Color = color; }

		virtual inline float GetIntensity() override { return m_Intensity; }
		virtual inline void SetIntensity(float intensity) override { m_Intensity = intensity; }

		virtual inline const std::string& GetName() override { return m_Name; }


	private:
		glm::vec3 m_Position;
		glm::vec3 m_Color;
		float m_Intensity;
	};
}
