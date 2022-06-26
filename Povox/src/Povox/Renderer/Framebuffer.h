#pragma once
#include "Image2D.h"

namespace Povox {

	struct FramebufferImageSpecification
	{
		FramebufferImageSpecification() = default;
		FramebufferImageSpecification(ImageFormat format)
			: Format(format) {}

		ImageFormat Format = ImageFormat::None;
		// TODO: Filtering/wrapping
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferImageSpecification> attachments)
			: Attachments(attachments) {}
		
		std::vector<FramebufferImageSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		struct
		{
			uint32_t X = 1.0f;
			uint32_t Y = 1.0f;
		} Scale;
		FramebufferAttachmentSpecification Attachements;
		std::vector<Ref<Image2D>> OriginalImages;

		uint32_t Samples = 1;

		std::string Name = "None";

		bool Resizable = true;
		bool SwapChainTarget = false;
		Ref<Framebuffer> OriginalFramebuffer = nullptr;
	};

	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int posX, int posY) = 0;

		virtual void ClearColorAttachment(size_t attachmentIndex, int value) = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
		virtual uint32_t GetDepthAttachmentRendererID() const = 0;

		virtual const Ref<Image2D> GetColorAttachment(size_t index = 0) const = 0;
		virtual const Ref<Image2D> GetDepthAttachment() const = 0;
		
		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}
