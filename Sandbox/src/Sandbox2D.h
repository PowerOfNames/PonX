#pragma once
#include <Povox.h>

class Sandbox2D : public Povox::Layer
{
public:
	Sandbox2D();
	~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(float deltatime) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Povox::Event & e) override;

private:
	Povox::OrthographicCameraController m_CameraController;

private:
	Povox::Ref<Povox::Texture2D> m_MapData;

	Povox::Ref<Povox::Shader> m_FlatColorShader;
	glm::vec4 m_SquareColor1 = { 32.0f / 255, 95.0f / 255, 83.0f / 255 , 1.0f };
	glm::vec2 m_SquarePos1 = { 0.0f, 0.0f };
};
