#include "pxpch.h"
#include "VulkanRendererAPI.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Povox {

	VulkanContext* VulkanRendererAPI::m_Context = nullptr;

	VulkanRendererAPI::~VulkanRendererAPI()
	{
		delete m_Context;
		PX_CORE_TRACE("Vulkan Renderer API delete!");
	}

	void VulkanRendererAPI::Init()
	{
	}

	void VulkanRendererAPI::Clear()
	{
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& clearColor)
	{
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		m_Context->OnFramebufferResize(width, height);
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
	}

	void VulkanRendererAPI::InitImGui()
	{
		m_Context->InitImGui();
	}

	void VulkanRendererAPI::BeginImGuiFrame()
	{
		m_Context->BeginImGuiFrame();
	}

	void VulkanRendererAPI::EndImGuiFrame()
	{
		m_Context->EndImGuiFrame();
	}

	void VulkanRendererAPI::AddImGuiImage(float width, float height)
	{
		m_Context->AddImGuiImage(width, height);
	}

}
