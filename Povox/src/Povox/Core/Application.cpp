#include "pxpch.h"
#include "Povox/Core/Application.h"
#include "Povox/Core/Input.h"
#include "Povox/Core/Log.h"
#include "Povox/Core/Core.h"


#include "Povox/Renderer/Renderer.h"


namespace Povox {

	Application* Application::s_Instance = nullptr;

	Application::Application() 
	{
		PX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Create();
		m_Window->SetEventCallback(PX_BIND_EVENT_FN(Application::OnEvent));
		
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);

		m_VertexArray.reset(VertexArray::Create());

		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f,
			 0.5f, -0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f,
			 0.0f,  0.5f, 0.0f, 0.8f, 0.2f, 0.6f, 1.0f
		};

		std::shared_ptr<VertexBuffer> vertexBuffer;
		vertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));
		BufferLayout layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[3] = {
			0, 1, 2
		};

		std::shared_ptr<IndexBuffer> indexBuffer;
		indexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		std::string vertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		
		layout(location = 1) in vec4 a_Color;		

		out vec4 v_Color;		

		void main()
		{
			gl_Position = vec4(a_Position, 1.0f);
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
		m_Shader.reset(Shader::Create(vertexSrc, fragmentSrc));

		m_SquareVertexArray.reset(VertexArray::Create());

		float squareVertices[3 * 4] = {
			-0.75f, -0.75f, 0.0f,
			 0.75f, -0.75f, 0.0f,
			 0.75f,  0.75f, 0.0f,
			-0.75f,  0.75f, 0.0f,
		};

		std::shared_ptr<VertexBuffer> squareVB;
		squareVB.reset(VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

		squareVB->SetLayout({ { ShaderDataType::Float3, "a_Position" } });
		m_SquareVertexArray->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = {
			0, 1, 2, 2, 3, 0
		};

		std::shared_ptr<IndexBuffer> squareIB;
		squareIB.reset(IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_SquareVertexArray->SetIndexBuffer(squareIB);

		std::string blueVertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		

		void main()
		{
			gl_Position = vec4(a_Position, 1.0f);
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
		m_WhiteShader.reset(Shader::Create(blueVertexSrc, blueFragmentSrc));
	}

	Application::~Application() 
	{
	}


	void Application::PushLayer(Layer* layer)
	{
		m_Layerstack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_Layerstack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(PX_BIND_EVENT_FN(Application::OnWindowClose));

		for (auto it = m_Layerstack.end(); it != m_Layerstack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			RenderCommand::SetClearColor({ 0.8f, 0.2f, 0.6f, 1.0f });
			RenderCommand::Clear();

			Renderer::BeginScene();
			m_WhiteShader->Bind();
			Renderer::Submit(m_SquareVertexArray);
			m_Shader->Bind();
			Renderer::Submit(m_VertexArray);
			Renderer::EndScene();

			for (Layer* layer : m_Layerstack)
			{
				layer->OnUpdate();
			}

			// To be executed on the Render thread
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_Layerstack)
			{
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}
