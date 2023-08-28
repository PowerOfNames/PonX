#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Utils/ShaderResources.h"

namespace Povox {

	class Framebuffer;
	class Pipeline;
	struct RenderPassSpecification
	{
		std::string DebugName = "Renderpass";

		Ref<Pipeline> Pipeline;


		Ref<Framebuffer> TargetFramebuffer = nullptr;
	};

	struct UniformBuffer;
	struct StorageBuffer;
	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void Recreate(uint32_t width, uint32_t height) = 0;

		virtual void BindResource(const std::string& name, Ref<UniformBuffer> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<StorageBuffer> resource) = 0;

		virtual void Bake() = 0;

		virtual const RenderPassSpecification& GetSpecification() const = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};


	class ComputePipeline;
	struct ComputePassSpecification
	{
		Ref<ComputePipeline> Pipeline = nullptr;

		std::string DebugName = "ComputePass";
	};

	class ComputePass
	{
	public:
		virtual ~ComputePass() = default;
		virtual void Recreate() = 0;

		virtual const ComputePassSpecification& GetSpecification() const = 0;

		//virtual Ref<Image2D> GetFinalImage(uint32_t index) = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<ComputePass> Create(const ComputePassSpecification& spec);
	};
}
