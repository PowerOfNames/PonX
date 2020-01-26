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
	std::shared_ptr<Povox::VertexArray> m_TriangleVertexArray;
	std::shared_ptr<Povox::VertexArray> m_SquareVertexArray;
	std::shared_ptr< Povox::Shader> m_WhiteShader;
	std::shared_ptr< Povox::Shader> m_BlankShader;
};

