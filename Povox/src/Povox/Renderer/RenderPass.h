#pragma once
#include "Povox/Core/Core.h"
#include "Povox/Renderer/Framebuffer.h"

namespace Povox {

	struct RenderPassSpecification
	{
		Ref<Framebuffer> TargetFramebuffer = nullptr;
		uint32_t ColorAttachmentCount = 0;
		bool HasDepthAttachment = false;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void Recreate() = 0;

		virtual const RenderPassSpecification& GetSpecification() const = 0;


		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};
}
