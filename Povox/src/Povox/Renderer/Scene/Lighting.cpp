#include "pxpch.h"
#include "Povox/Renderer/Scene/Lighting.h"



namespace Povox {

	Light::Light(const std::string& name)
		:m_Name(name)
	{
	}

	PointLight::PointLight(const std::string& name, const glm::vec3& position, const glm::vec3& color, float intensity)
		:Light(name), m_Position(position), m_Color(color), m_Intensity(intensity)
	{
	}
}