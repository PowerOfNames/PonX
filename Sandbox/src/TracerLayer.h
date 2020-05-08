#pragma once

#include <Povox.h>

class TracerLayer : public Povox::Layer
{
public:
	TracerLayer();
	~TracerLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(float deltatime) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Povox::Event& e) override;

private:
	Povox::PerspectiveCameraController m_CameraController;

private:
	glm::vec3* m_CameraPosition;
	float m_DeltaTime = 0.0f;
};
