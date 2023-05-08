#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Core/Window.h"
#include "Povox/Events/Event.h"
#include "Povox/Events/ApplicationEvent.h"
#include "Povox/Core/LayerStack.h"
#include "Povox/Renderer/RendererAPI.h"

#include "Povox/ImGui/ImGuiLayer.h"
#include "Povox/ImGui/ImGuiVulkanLayer.h"


namespace Povox {

	struct ApplicationState
	{
		bool WindowInitialized = false;
		bool RendererInitialized = false;

		bool ApplicationInitialized = false;
	};
	struct ApplicationSpecification
	{
		ApplicationState State{};

		RendererAPI::API UseAPI = RendererAPI::API::NONE;
		bool ImGuiEnabled = true;

		uint32_t MaxFramesInFlight = 1;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& specs);
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }
		ImGuiVulkanLayer* GetImGuiVulkanLayer() { return m_ImGuiVulkanLayer; }

		inline Window& GetWindow() { return *m_Window; }
		inline static Application* Get() {	return s_Instance; }
		inline ApplicationSpecification& GetSpecification() { return m_Specification; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		bool OnFramebufferResize(FramebufferResizeEvent& e);
		
		ApplicationSpecification m_Specification;

		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		ImGuiVulkanLayer* m_ImGuiVulkanLayer;
		bool m_Running = true;
		bool m_Minimized = false;
		LayerStack m_Layerstack;
		float m_DeltaTime = 0.0f;

		static Application* s_Instance;
	};

	// To be defined in CLIENT	
	Application* CreateApplication(int argc, char** argv);

}
