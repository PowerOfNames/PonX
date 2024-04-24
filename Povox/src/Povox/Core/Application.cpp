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


		PX_CORE_WARN("Application::Application: Starting initialization...");

		PX_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		/* Correct order to initialize the core :
		 *
		 * - First set the correct RendererAPI
		 * - Create and setup Window -> RendererContext and Swapchain
		 * - Setup RendererAPI -> Dependent on finished Context and Swapchain
		 */

		RendererAPI::SetAPI(specs.UseAPI);

		WindowSpecification windowSpecs{};
		windowSpecs.Title = "Povosom";
		windowSpecs.Width = 1600;
		windowSpecs.Height = 900;
		m_Window = Window::Create(windowSpecs);
		PX_CORE_ASSERT(m_Window, "Failed to create Window!");
		m_Window->SetEventCallback(PX_BIND_EVENT_FN(Application::OnEvent));
		m_Specification.State.WindowInitialized = m_Window->Init();

		PX_CORE_INFO("Initializing Renderer...");

		RendererSpecification rendererSpecs{};
		rendererSpecs.State.WindowWidth = windowSpecs.Width;
		rendererSpecs.State.WindowHeight = windowSpecs.Height;
		rendererSpecs.State.ViewportWidth = windowSpecs.Width;
		rendererSpecs.State.ViewportHeight = windowSpecs.Height;

		rendererSpecs.MaxSceneObjects = 20000;
		rendererSpecs.MaxFramesInFlight = specs.MaxFramesInFlight;		
		m_Specification.State.RendererInitialized = Renderer::Init(rendererSpecs);

		PX_CORE_INFO("Completed Renderer initialization.");
		PX_CORE_INFO("Pushing (Im)GUI Layer...");

		//The ImGui stuff should not be in the application -> GUI Renderer should take care of this stuff
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::NONE:
			{
				PX_CORE_WARN("RendererAPI:None -> No GUI Layer can be pushed!");
				PX_CORE_ASSERT(true, "Failed to push GUI Layer -> No API selected.")
			}
			case RendererAPI::API::Vulkan:
			{
				PX_CORE_INFO("Selected API: Vulkan -> Pushing ImGuiVulkanLayer...");
				
				m_ImGuiVulkanLayer = new ImGuiVulkanLayer();
				PX_CORE_ASSERT(m_ImGuiVulkanLayer, "Failed to create ImGuiVulkanLayer");
				PushOverlay(m_ImGuiVulkanLayer);

				PX_CORE_INFO("Selected API: Vulkan -> Pushed ImGuiVulkanLayer.");
				break;
			}
			default:
			PX_CORE_ASSERT(false, "This API is not supported!");
		}		
		
		m_Specification.State.ApplicationInitialized = true;
		PX_CORE_INFO("Pushed (Im)GUI Layer.");
		PX_CORE_WARN("Application::Application: Completed initialization.");
	}


	void Application::Run()
	{
		PX_PROFILE_FUNCTION();

		//Renderer::AddTimestampQuery("GUIPass", 2);
		PX_CORE_INFO("Application::Run: Starting Main-Loop...");
		while (m_Running)
		{
			PX_PROFILE_SCOPE("Application Run-Loop");

			ExecuteMainThreadQueue();

			//Between Begin and EndFrame happens the CommandRecording of everything rendering related
			if (!m_Minimized)
			{
				if (!Renderer::BeginFrame())
					continue;

				float time = (float)glfwGetTime();
				Timestep timestep = time - m_DeltaTime;
				m_DeltaTime = time;

				for (Layer* layer : m_Layerstack)
				{
					layer->OnUpdate(timestep);//contains begin renderpass, begin commandbuffer, end renderpass, end commandbuffer
				}

				//if the GUI rendering happens here, then I have to change the frame handling and end it AFTER ImGui or other GUI has been handled and rendered before buffer swapping
				if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan && m_Specification.ImGuiEnabled)
				{
					PX_PROFILE_SCOPE("ImGui Update and render loop");
					const void* imGuiCmd = Renderer::GetGUICommandBuffer(Renderer::GetCurrentFrameIndex());
					Renderer::BeginCommandBuffer(imGuiCmd);
					Renderer::StartTimestampQuery("GUIPass");
					Renderer::BeginGUIRenderPass();
					m_ImGuiVulkanLayer->Begin();
					for (Layer* layer : m_Layerstack)
					{
						layer->OnImGuiRender();
					}
					m_ImGuiVulkanLayer->End(); // all the objects that got created during OnImgUiRender now are put into ImGuis DrawList 
					Renderer::DrawGUI();
					Renderer::EndGUIRenderPass();
					Renderer::StopTimestampQuery("GUIPass");
					Renderer::EndCommandBuffer();
				}
				Renderer::EndFrame();

				m_Window->OnUpdate();
			}
			else {
				PX_CORE_INFO("Application::Run: Minimized");
			}
			
			m_Window->PollEvents();
		}
		PX_CORE_INFO("Application::Run: Stopped Main-Loop.");
	}

	Application::~Application()
	{
		PX_PROFILE_FUNCTION();

		PX_CORE_INFO("Application::~Application: Starting shutdown...");
		
		PX_CORE_INFO("Starting Renderer shutdown...");
		Renderer::WaitForDeviceFinished();
		Renderer::Shutdown();

		PX_CORE_INFO("Completed Renderer shutdown.");


		m_Window->Close();
		PX_CORE_INFO("Application::~Application: Completed shutdown...");
	}

	void Application::PushLayer(Layer* layer)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Application::PushLayer: Pushing '{0}' on layerStack...", layer->GetDebugName());
		
		m_Layerstack.PushLayer(layer);
		
		PX_CORE_INFO("Application::PushLayer: Performing OnAttach of '{0}'...", layer->GetDebugName());		
		
		layer->OnAttach();

		PX_CORE_INFO("Application::PushLayer: Attached '{0}'.", layer->GetDebugName());
		PX_CORE_INFO("Application::PushLayer: Pushed '{0}'.", layer->GetDebugName());
	}

	void Application::PushOverlay(Layer* overlay)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Application::PushOverlay: Pushing '{0}' on layerStack...", overlay->GetDebugName());

		m_Layerstack.PushOverlay(overlay);

		PX_CORE_INFO("Application::PushOverlay: Performing OnAttach of '{0}'...", overlay->GetDebugName());

		overlay->OnAttach();

		PX_CORE_INFO("Application::PushOverlay: Attached '{0}'.", overlay->GetDebugName());
		PX_CORE_INFO("Application::PushOverlay: Pushed '{0}'.", overlay->GetDebugName());
	}

	/**
	 * First calls OnEventCallbacks in Application:
	 * - Window
	 * - Renderer
	 * 
	 * Then reverse-iterates over LayerStack and calls OnEvent() per Layer
	 */
	void Application::OnEvent(Event& e)
	{
		PX_PROFILE_FUNCTION();


		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(PX_BIND_EVENT_FN(Application::OnWindowClose)); 
		dispatcher.Dispatch<FramebufferResizeEvent>(PX_BIND_EVENT_FN(Application::OnFramebufferResize));

		// FBResize give pixel values, window screen space values -> pixel doesnot always equal screen space, e.g. retina
		//dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(Application::OnWindowResize));

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
		PX_CORE_ERROR("Application::OnWindowResize to {0};{1}", e.GetWidth(), e.GetHeight());

		m_Window->OnResize(e.GetWidth(), e.GetHeight());
		return false;
	}

	bool Application::OnFramebufferResize(FramebufferResizeEvent& e)
	{
		PX_PROFILE_FUNCTION();


		if (e.GetWidth() <= 0 || e.GetHeight() <= 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;

		m_Window->OnResize(e.GetWidth(), e.GetHeight());
		Renderer::OnResize(e.GetWidth(), e.GetHeight());

		return false;
	}	

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::AddMainThreadQueueInstruction(const std::function<void()>& func)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		m_MainThreadQueue.emplace_back(func);
	}

	void Application::ExecuteMainThreadQueue()
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}

}
