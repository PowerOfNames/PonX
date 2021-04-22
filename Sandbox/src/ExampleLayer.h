#pragma once
#include <Povox.h>


class ExampleLayer : public Povox::Layer
{
public:
	ExampleLayer();
	~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Povox::Timestep deltatime) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Povox::Event& e) override;

private:
	Povox::OrthographicCameraController m_CameraController;

private:
	Povox::Ref<Povox::VertexArray> m_TriangleVertexArray;
	Povox::Ref<Povox::VertexArray> m_SquareVertexArray;
	Povox::Ref<Povox::Shader> m_WhiteShader, m_FlatColorShader, m_TextureShader;
	Povox::Ref<Povox::Texture> m_Logo;

	glm::vec4 m_SquareColor1 = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f};
	glm::vec4 m_SquareColor2 = { 0.2f, 0.2f, 0.2f, 1.0f};
	glm::vec3 m_White = { 1.0f, 1.0f, 1.0f };

	glm::vec3 m_LogoPosition = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_LogoScale = { 1.0f, 1.0f, 1.0f };
};

