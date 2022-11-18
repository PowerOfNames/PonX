#pragma once
#include "Povox/Core/Core.h"
#include "Povox/Renderer/Framebuffer.h"

namespace Povox {

	struct RenderPassSpecification
	{
		Ref<Framebuffer> TargetFramebuffer;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;


		virtual const RenderPassSpecification& GetSpecification() const = 0;


		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};
}
