#pragma once

#include "Povox/Renderer/Framebuffer.h"

namespace Povox {

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		// state of the framebuffer not valid -> recreate it
		void Invalidate();
		
		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		inline virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
		inline virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
		inline virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }
	private:
		uint32_t m_RendererID;
		uint32_t m_ColorAttachment, m_DepthAttachment;
		FramebufferSpecification m_Specification;

	};

}
