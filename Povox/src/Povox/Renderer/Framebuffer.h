#pragma once


namespace Povox {

	struct FramebufferSpecification
	{
		uint32_t Width, Height;
		// FramebufferFormat Format;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class Framebuffer
	{
	public:
		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);


		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;
		virtual uint32_t GetColorAttachmentRendererID() const = 0;
		virtual uint32_t GetDepthAttachmentRendererID() const = 0;
	};

}
