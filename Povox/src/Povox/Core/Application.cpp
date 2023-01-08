#include "pxpch.h"
#include "Povox/Core/Application.h"

#include "Povox/Core/Log.h"
#include "Povox/Core/Input.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Core/Timestep.h"

#include <GLFW/glfw3.h>


namespace Povox {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specs)
		: m_Specification(specs)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		/* Correct order to initialize the core :
		 *
		 * - First set the correct RendererAPI
		 * - Create and setup Window -> RendererContext and Swapchain
		 * - Setup RendererAPI -> Dependent on finished Context and Swapchain
		 */

		RendererAPI::SetAPI(specs.UseAPI);

		m_Window = Window::Create();
		m_Window->SetEventCallback(PX_BIND_EVENT_FN(Application::OnEvent));

		RendererSpecification rendererSpecs{};
		rendererSpecs.MaxSceneObjects = 20000;
		rendererSpecs.MaxFramesInFlight = specs.MaxFramesInFlight;
		Renderer::Init(rendererSpecs);

		//The ImGui stuff should not be in the application -> GUI Renderer should take care of this stuff
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				m_ImGuiVulkanLayer = new ImGuiVulkanLayer();
				PushOverlay(m_ImGuiVulkanLayer);
				break;
			}
			default:
			PX_CORE_ASSERT(false, "This API is not supported!");
		}		
	}


	void Application::Run()
	{
		PX_PROFILE_FUNCTION();


		while (m_Running)
		{
			PX_PROFILE_SCOPE("Application Run-Loop");

			//Between Begin and EndFrame happens the CommandRecording of everything rendering related
			Renderer::BeginFrame();

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_DeltaTime;
			m_DeltaTime = time;

			if (!m_Minimized)
			{
				for (Layer* layer : m_Layerstack)
				{
					layer->OnUpdate(timestep);
				}
			}

			//if the GUI rendering happens here, then I have to change the frame handling and end it AFTER ImGui or other GUI has been handled and rendered before buffer swapping
			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan && m_Specification.ImGuiEnabled)
			{
				m_ImGuiVulkanLayer->Begin();
				for (Layer* layer : m_Layerstack)
				{
					layer->OnImGuiRender();
				}
				m_ImGuiVulkanLayer->End(); // the recorded commands should no w be added to the commandbuffer vctor, which then is processed during Swapchain::submit
			}

			
			//The recorded commands now get processed in the swapchain, which lives in the window
			m_Window->OnUpdate();

			Renderer::EndFrame();

			PX_CORE_WARN("Finished Run!");
		}
	}

	Application::~Application() 
	{
		PX_PROFILE_FUNCTION();


		/* Shutdown routine:
		 * end last rendering
		 * shutdown the RendererAPI -> automatic
		 * shutdown Swapchain and Context
		 * shutdown window
		 * close app
		 */

		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		PX_PROFILE_FUNCTION();


		m_Layerstack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		PX_PROFILE_FUNCTION();


		m_Layerstack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		PX_PROFILE_FUNCTION();


		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(PX_BIND_EVENT_FN(Application::OnWindowClose)); 
		dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_Layerstack.rbegin(); it != m_Layerstack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		PX_PROFILE_FUNCTION();

		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();


		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		m_Window->OnResize(e.GetWidth(), e.GetHeight());
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	bool Application::OnFramebufferResize(FramebufferResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();


		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::OnFramebufferResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void Application::Close()
	{
		m_Running = false;
	}
}
