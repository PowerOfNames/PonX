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

		m_Window = Window::Create();
		m_Window->SetEventCallback(PX_BIND_EVENT_FN(Application::OnEvent));
		PX_CORE_ASSERT(m_Window, "Failed to create Window!");

		PX_CORE_INFO("Initializing Renderer...");

		RendererSpecification rendererSpecs{};
		rendererSpecs.MaxSceneObjects = 20000;
		rendererSpecs.MaxFramesInFlight = specs.MaxFramesInFlight;
		Renderer::Init(rendererSpecs);

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
		

		PX_CORE_INFO("Pushed (Im)GUI Layer.");
		PX_CORE_WARN("Application::Application: Completed initialization.");
	}


	void Application::Run()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("Application::Run: Starting Main-Loop...");
		while (m_Running)
		{
			PX_PROFILE_SCOPE("Application Run-Loop");

			//Between Begin and EndFrame happens the CommandRecording of everything rendering related
			if (!Renderer::BeginFrame())
				continue;

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_DeltaTime;
			m_DeltaTime = time;

			if (!m_Minimized)
			{
				for (Layer* layer : m_Layerstack)
				{
					layer->OnUpdate(timestep);//contains begin renderpass, begin commandbuffer, end rnderpass, end commandbuffer
				}

				//if the GUI rendering happens here, then I have to change the frame handling and end it AFTER ImGui or other GUI has been handled and rendered before buffer swapping
				if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan && m_Specification.ImGuiEnabled)
				{
					const void* imGuiCmd = Renderer::GetGUICommandBuffer(Renderer::GetCurrentFrameIndex());
					Renderer::BeginCommandBuffer(imGuiCmd);
					Renderer::BeginGUIRenderPass();
					m_ImGuiVulkanLayer->Begin();
					for (Layer* layer : m_Layerstack)
					{
						layer->OnImGuiRender();
					}
					m_ImGuiVulkanLayer->End(); // all the objects that got created during OnImgUiRender now are put into ImGuis DrawList 
					Renderer::DrawGUI();
					Renderer::EndGUIRenderPass();
					Renderer::EndCommandBuffer();
				}
			}
			Renderer::EndFrame();
			
			m_Window->OnUpdate();
		}
		PX_CORE_INFO("Application::Run: Stopped Main-Loop.");
	}

	Application::~Application() 
	{
		PX_PROFILE_FUNCTION();

		PX_CORE_INFO("Application::~Application: Starting shutdown...");
		/* Shutdown routine:
		 * end last rendering
		 * shutdown the RendererAPI -> automatic
		 * shutdown Swapchain and Context
		 * shutdown window
		 * close app
		 */
		delete m_ImGuiVulkanLayer;
		
		PX_CORE_INFO("Starting Renderer shutdown...");
		
		Renderer::Shutdown();

		PX_CORE_INFO("Completed Renderer shutdown.");
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

	void Application::OnEvent(Event& e)
	{
		PX_PROFILE_FUNCTION();


		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(PX_BIND_EVENT_FN(Application::OnWindowClose)); 
		dispatcher.Dispatch<WindowResizeEvent>(PX_BIND_EVENT_FN(Application::OnWindowResize));
		//dispatcher.Dispatch<FramebufferResizeEvent>(PX_BIND_EVENT_FN(Application::OnFramebufferResize));

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
		//Renderer::FramebufferResized(e.GetWidth(), e.GetHeight());
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
		PX_CORE_ERROR("Application::OnFramebufferResize to {0};{1}", e.GetWidth(), e.GetHeight());

		m_Minimized = false;

		return false;
	}

	void Application::Close()
	{
		m_Running = false;
	}
}
