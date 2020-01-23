#include "pxpch.h"
#include "Povox/Core/Application.h"

#include "Povox/Core/Log.h"
#include "Povox/Core/Core.h"

#include "Povox/Core/Input.h"

#include <glad/glad.h>

#include <glm/glm.hpp>


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

		glGenVertexArrays(1, &m_VertexArray);
		glBindVertexArray(m_VertexArray);

		float vertices[3 * 3] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.0f,  0.5f, 0.0f
		};

		m_VertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));
		m_VertexBuffer->Bind();

		uint32_t indices[3] = {
			0, 1, 2
		};

		m_IndexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_IndexBuffer->Bind();

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

		std::string vertexSrc = R"(
		#version 330 core
		
		layout(location = 0) in vec3 a_Position;		

		void main()
		{
			gl_Position = vec4(a_Position, 1.0f);
		}
		)";

		std::string fragmentSrc = R"(
		#version 330 core
		
		layout(location = 0) out vec4 color;		

		void main()
		{
			color = vec4(32.0f / 255, 93.0f / 255, 85.0f / 255, 1.0f);
		}
		)";
		m_Shader.reset(Shader::Create(vertexSrc, fragmentSrc));
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
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			m_Shader->Bind();

			glBindVertexArray(m_VertexArray);
			glDrawElements(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);

			
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
