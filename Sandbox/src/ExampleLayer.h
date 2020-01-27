#pragma once
#include <Povox.h>

class ExampleLayer : public Povox::Layer
{
public:
	ExampleLayer();
	~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(float deltatime) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Povox::Event& e) override;

private:
	Povox::OrthographicCameraController m_CameraController;
	float m_ts = 0.16f;

private:
	Povox::Ref<Povox::VertexArray> m_TriangleVertexArray;
	Povox::Ref<Povox::VertexArray> m_SquareVertexArray;
	Povox::Ref<Povox::Shader> m_WhiteShader;
	Povox::Ref<Povox::Shader> m_FlatColorShader;

	glm::vec3 m_SquareColor1 = { 32.0f / 255, 93.0f / 255, 83.0f / 255 };
	glm::vec3 m_SquareColor2 = { 0.2f, 0.2f, 0.2f };
};

