#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Core/Window.h"
#include "Povox/Events/Event.h"
#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Core/LayerStack.h"

#include "Povox/ImGui/ImGuiLayer.h"
#include "Povox/ImGui/ImGuiVulkanLayer.h"

#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/OrthographicCameraController.h"


namespace Povox {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
		ImGuiVulkanLayer* GetImGuiVulkanLayer() { return m_ImGuiVulkanLayer; }

		inline Window& GetWindow() const { return *m_Window; }
		inline static Application& Get() {	return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		ImGuiVulkanLayer* m_ImGuiVulkanLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_Layerstack;
		float m_DeltaTime = 0.0f;

	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT	
	Application* CreateApplication();

}
