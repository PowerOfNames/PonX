#pragma once
#include <Povox.h>

class ExampleLayer : public Povox::Layer
{
public:
	ExampleLayer();
	~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Povox::Timestep deltatime) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Povox::Event& e) override;

private:
	Povox::EditorCamera m_EditorCamera;

	Povox::Ref<Povox::Pipeline> m_GeometryPipeline;
	Povox::Ref<Povox::Pipeline> m_ColorPipeline;
	Povox::Ref<Povox::Pipeline> m_CompositePipeline;

	bool m_DemoActive = false;

	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportBounds[2];
	bool m_ViewportIsFocused = false;
	bool m_ViewportIsHovered = false;

};

