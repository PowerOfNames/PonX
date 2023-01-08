#pragma once
#include "Image2D.h"

namespace Povox {

	struct FramebufferImageSpecification
	{
		/*FramebufferImageSpecification() = default;
		FramebufferImageSpecification(ImageFormat format)
			: Format(format) {}*/

		ImageFormat Format = ImageFormat::None;
		MemoryUtils::MemoryUsage Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		ImageTiling Tiling = ImageTiling::LINEAR;
		std::vector<ImageUsage> Usages = { ImageUsage::COLOR_ATTACHMENT };
		// TODO: Filtering/wrapping
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferImageSpecification> attachments)
			: Attachments(attachments) {}
		
		std::vector<FramebufferImageSpecification> Attachments;
	};

	class Framebuffer;
	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		struct
		{
			float X = 1.0f;
			float Y = 1.0f;
		} Scale;
		FramebufferAttachmentSpecification Attachments;
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

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int posX, int posY) = 0;


		virtual const FramebufferSpecification& GetSpecification() const = 0;

		virtual const std::vector<Ref<Image2D>>& GetColorAttachments() = 0;
		virtual const Ref<Image2D> GetColorAttachment(size_t index = 0) = 0;
		virtual const Ref<Image2D> GetDepthAttachment() = 0;
		
		static Ref<Framebuffer> Create();
		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}
