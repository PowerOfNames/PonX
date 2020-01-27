#include "ExampleLayer.h"

#include "Platform/OpenGL/OpenGLShader.h"
#include <ImGui/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>



ExampleLayer::ExampleLayer()
	: Layer("Example"), m_CameraController(1280.0f / 720.0f, true)
{
	m_TriangleVertexArray.reset(Povox::VertexArray::Create());

	float vertices[3 * 7] = {
		-0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f,
		 0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f,
		 0.0f,  0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f
	};

	Povox::Ref<Povox::VertexBuffer> vertexBuffer;
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

	Povox::Ref<Povox::IndexBuffer> indexBuffer;
	indexBuffer.reset(Povox::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
	m_TriangleVertexArray->SetIndexBuffer(indexBuffer);

	std::string vertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		
		layout(location = 1) in vec4 a_Color;		

		uniform mat4 u_ViewProjection;
		uniform mat4 u_Transform;

		out vec4 v_Color;		

		void main()
		{
			gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0f);
			v_Color = a_Color;
		}
		)";

	std::string fragmentSrc = R"(
		#version 330 core
		
		layout(location = 0) out vec4 color;

		in vec4 v_Color;	

		void main()
		{
			color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		)";
	m_WhiteShader.reset(Povox::Shader::Create(vertexSrc, fragmentSrc));

	m_SquareVertexArray.reset(Povox::VertexArray::Create());

	float squareVertices[3 * 4] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
	};

	Povox::Ref<Povox::VertexBuffer> squareVB;
	squareVB.reset(Povox::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

	squareVB->SetLayout({ { Povox::ShaderDataType::Float3, "a_Position" } });
	m_SquareVertexArray->AddVertexBuffer(squareVB);

	uint32_t squareIndices[6] = {
		0, 1, 2, 2, 3, 0
	};

	Povox::Ref<Povox::IndexBuffer> squareIB;
	squareIB.reset(Povox::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
	m_SquareVertexArray->SetIndexBuffer(squareIB);

	std::string flatVertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		

		uniform mat4 u_ViewProjection;
		uniform mat4 u_Transform;

		void main()
		{
			gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0f);

		}
		)";

	std::string flatFragmentSrc = R"(
		#version 330 core
		
		layout(location = 0) out vec4 color;

		uniform vec3 u_Color;

		void main()
		{
			color = vec4(u_Color, 1.0f);
		}
		)";
	m_FlatColorShader.reset(Povox::Shader::Create(flatVertexSrc, flatFragmentSrc));
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

	static glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

	m_FlatColorShader->Bind();

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 20; x++)
		{
			glm::vec3 pos(x * 0.1f, y * 0.1f, 0.0f);
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
			
			if( (x % 2 == 0) && !( y % 2 == 0 ) || !(x % 2 == 0) && (y % 2 == 0))
				std::dynamic_pointer_cast<Povox::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat3("u_Color", m_SquareColor1);
			else 
				std::dynamic_pointer_cast<Povox::OpenGLShader>(m_FlatColorShader)->UploadUniformFloat3("u_Color", m_SquareColor2);
			
			Povox::Renderer::Submit(m_FlatColorShader, m_SquareVertexArray, transform);
		}
	}
	Povox::Renderer::EndScene();
}


void ExampleLayer::OnImGuiRender()
{
	{
		ImGui::Begin("Square1");
		ImGui::ColorPicker3("Square1ColorPicker", &m_SquareColor1.r);
		ImGui::End();

		ImGui::Begin("Square2");
		ImGui::ColorPicker3("Square2ColorPicker", &m_SquareColor2.r);
		ImGui::End();
	}
}

void ExampleLayer::OnEvent(Povox::Event& e)
{
	m_CameraController.OnEvent(e);
}