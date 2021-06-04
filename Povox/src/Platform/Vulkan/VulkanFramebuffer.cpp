#include "pxpch.h"
#include "VulkanFramebuffer.h"


namespace Povox {

	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec)
	{

	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{

	}

	void VulkanFramebuffer::Bind() const
	{
	}

	void VulkanFramebuffer::Unbind() const
	{
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{

	}

	void VulkanFramebuffer::ClearColorAttachment(uint32_t attachmentIndex, int value)
	{
		PX_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "OGLFramebuffer::ReadPixel - Index out of scope!");

	}

	int VulkanFramebuffer::ReadPixel(uint32_t attachmentIndex, int posX, int posY)
	{
		PX_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "OGLFramebuffer::ReadPixel - Index out of scope!");

		return 0;
	}


}