#pragma once
#include "Platform/Vulkan/VulkanFramebuffer.h"

#include "Povox/Core/Core.h"

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
