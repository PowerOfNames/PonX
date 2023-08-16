#pragma once
#include "Povox/Core/Core.h"

namespace Povox {

	class Framebuffer;
	class Pipeline;
	struct RenderPassSpecification
	{
		std::string DebugName = "Renderpass";

		Ref<Pipeline> Pipeline = nullptr;
		Ref<Framebuffer> TargetFramebuffer = nullptr;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void Recreate(uint32_t width, uint32_t height) = 0;

		virtual const RenderPassSpecification& GetSpecification() const = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};



	struct ComputePassSpecification
	{


		std::string DebugName = "ComputePass";
	};

	class ComputePass
	{
	public:
		virtual ~ComputePass() = default;
		virtual void Recreate() = 0;

		virtual const ComputePassSpecification& GetSpecification() const = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<ComputePass> Create(const ComputePassSpecification& spec);
	};
}
