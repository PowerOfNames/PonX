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


};

