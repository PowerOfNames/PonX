#pragma once

#include <Povox.h>

class VoxelExample : public Povox::Layer 
{
public:
	VoxelExample();
	~VoxelExample() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Povox::Timestep deltatime) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Povox::Event& e) override;

private:
	Povox::PerspectiveCameraController m_CameraController;

private:
	Povox::Ref<Povox::VertexArray> m_SquareVertexArray;
	Povox::Ref<Povox::Texture2D> m_TextureLogo;

	glm::vec3 m_CameraPosition;

	Povox::Ref<Povox::Shader> m_FlatColorShader;
	glm::vec4 m_SquareColor1 = { 32.0f / 255, 95.0f / 255, 83.0f / 255 , 1.0f };
};
