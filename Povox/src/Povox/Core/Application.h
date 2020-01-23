#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Core/Window.h"
#include "Povox/Events/Event.h"
#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Core/LayerStack.h"
#include "Povox/ImGui/ImGuiLayer.h"



namespace Povox {

	class POVOX_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		inline Window& GetWindow() const { return *m_Window; }

		inline static Application& Get() {	return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		LayerStack m_Layerstack;

	private:
		static Application* s_Instance;

	private:
		unsigned int m_VertexArray, m_VertexBuffer, m_IndexBuffer;
	};

	// To be defined in CLIENT	
	Application* CreateApplication();

}
