#pragma once
#include "Povox/Renderer/Framebuffer.h"


namespace Povox {

	class VulkanFramebuffer : public Framebuffer
	{
	public:
		VulkanFramebuffer(const FramebufferSpecification& spec);
		virtual ~VulkanFramebuffer();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int posX, int posY) override;

		virtual void ClearColorAttachment(uint32_t attachmentIndex, int value) override;

		inline virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
		inline virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { PX_CORE_ASSERT(index < m_ColorAttachments.size(), "OGLFramebuffer::GetColorAttID - Index out of scope!"); return m_ColorAttachments[index]; }
		inline virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

	private:
		uint32_t m_RendererID = 0;
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification;

		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment;

	};
}