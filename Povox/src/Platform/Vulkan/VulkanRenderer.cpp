#include "pxpch.h"
#include "VulkanRenderer.h"


#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Povox {

	VulkanContext* VulkanRenderer::m_Context = nullptr;

	VulkanRenderer::~VulkanRenderer()
	{
		delete m_Context;
		PX_CORE_TRACE("Vulkan Renderer API delete!");
	}

	void VulkanRenderer::Init()
	{
		m_ImGui = CreateScope<VulkanImGui>(&m_UploadContext, Application::Get().GetWindow().GetSwapchain().GetImageFormat(), (uint8_t)Application::GetSpecification().RendererProps.MaxFramesInFlight);

	}

	void VulkanRenderer::Shutdown()
	{
		VkDevice device = m_Context->GetDevice()->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		m_ImGui->DestroyFramebuffers();
		m_ImGui->DestroyRenderPass();
		m_ImGui->Destroy();

	}

	void VulkanRenderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		OnResize(width, height);
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		PX_CORE_WARN("Start swapchain recreation!");
			

		vkDeviceWaitIdle(m_Context->GetDevice()->GetVulkanDevice());

		CreateDepthResources();
		//managesd by whoever is createing the viewports, except for when the renderer is holding all the viewports, and here goes over them and recreates the swapchain dependent viewports
		for (auto& framebuffer : m_OffscreenFramebuffers)
		{
			framebuffer.Resize(width, height);
		}
		VkExtent2D extent{ Application::Get().GetWindow().GetSwapchain().GetProperties().Width, Application::Get().GetWindow().GetSwapchain().GetProperties().Height };
		m_ImGui->OnSwapchainRecreate(Application::Get().GetWindow().GetSwapchain().GetImageViews(), extent);
	}

	void VulkanRenderer::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
	}

	void VulkanRenderer::InitImGui()
	{
		m_Context->InitImGui();
	}

	void VulkanRenderer::BeginImGuiFrame()
	{
		m_Context->BeginImGuiFrame();
	}

	void VulkanRenderer::EndImGuiFrame()
	{
		m_Context->EndImGuiFrame();
	}

	void VulkanRenderer::AddImGuiImage(float width, float height)
	{
		m_Context->AddImGuiImage(width, height);
	}

}
