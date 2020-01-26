#include "ExampleLayer.h"

#include <ImGui/imgui.h>

ExampleLayer::ExampleLayer()
	: Layer("Example"), m_CameraController(1260 / 980)
{
	m_TriangleVertexArray.reset(Povox::VertexArray::Create());

	float vertices[3 * 7] = {
		-0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f,
		 0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f,
		 0.0f,  0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f
	};

	std::shared_ptr<Povox::VertexBuffer> vertexBuffer;
	vertexBuffer.reset(Povox::VertexBuffer::Create(vertices, sizeof(vertices)));
	Povox::BufferLayout layout = {
		{ Povox::ShaderDataType::Float3, "a_Position" },
		{ Povox::ShaderDataType::Float4, "a_Color" }
	};
	vertexBuffer->SetLayout(layout);
	m_TriangleVertexArray->AddVertexBuffer(vertexBuffer);

	uint32_t indices[3] = {
		0, 1, 2
	};

	std::shared_ptr<Povox::IndexBuffer> indexBuffer;
	indexBuffer.reset(Povox::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
	m_TriangleVertexArray->SetIndexBuffer(indexBuffer);

	std::string vertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		
		layout(location = 1) in vec4 a_Color;		

		uniform mat4 u_ViewProjection;

		out vec4 v_Color;		

		void main()
		{
			gl_Position = u_ViewProjection * vec4(a_Position, 1.0f);
			v_Color = a_Color;
		}
		)";

	std::string fragmentSrc = R"(
		#version 330 core
		
		layout(location = 0) out vec4 color;

		in vec4 v_Color;	

		void main()
		{
			color = v_Color;
			color = vec4(32.0f / 255, 93.0f / 255, 85.0f / 255, 1.0f);
		}
		)";
	m_BlankShader.reset(Povox::Shader::Create(vertexSrc, fragmentSrc));

	m_SquareVertexArray.reset(Povox::VertexArray::Create());

	float squareVertices[3 * 4] = {
		-0.75f, -0.75f, 0.0f,
		 0.75f, -0.75f, 0.0f,
		 0.75f,  0.75f, 0.0f,
		-0.75f,  0.75f, 0.0f,
	};

	std::shared_ptr<Povox::VertexBuffer> squareVB;
	squareVB.reset(Povox::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

	squareVB->SetLayout({ { Povox::ShaderDataType::Float3, "a_Position" } });
	m_SquareVertexArray->AddVertexBuffer(squareVB);

	uint32_t squareIndices[6] = {
		0, 1, 2, 2, 3, 0
	};

	std::shared_ptr<Povox::IndexBuffer> squareIB;
	squareIB.reset(Povox::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
	m_SquareVertexArray->SetIndexBuffer(squareIB);

	std::string blueVertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		
	
		uniform mat4 u_ViewProjection;

		void main()
		{
			gl_Position = u_ViewProjection * vec4(a_Position, 1.0f);
		}
		)";

	std::string blueFragmentSrc = R"(
		#version 330 core
		
		layout(location = 0) out vec4 color;

		void main()
		{
			color = vec4(0.3, 0.1, 0.7, 1.0);
		}
		)";
	m_WhiteShader.reset(Povox::Shader::Create(blueVertexSrc, blueFragmentSrc));
}

void ExampleLayer::OnAttach()
{
}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate(float deltatime)
{
	m_CameraController.OnUpdate(deltatime);
	
	Povox::RenderCommand::SetClearColor({ 0.8f, 0.2f, 0.6f, 1.0f });
	Povox::RenderCommand::Clear();
	
	Povox::Renderer::BeginScene(m_CameraController.GetCamera());

	Povox::Renderer::Submit(m_WhiteShader, m_SquareVertexArray);
	Povox::Renderer::Submit(m_BlankShader, m_TriangleVertexArray);

	Povox::Renderer::EndScene();
}


void ExampleLayer::OnImGuiRender()
{
	{
		ImGui::Begin("Test");
		ImGui::Text("Hello Povox User!");
		ImGui::End();
	}
}

void ExampleLayer::OnEvent(Povox::Event& e)
{
	m_CameraController.OnEvent(e);
}