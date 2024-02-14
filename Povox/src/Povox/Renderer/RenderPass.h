#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Utils/ShaderResource.h"

namespace Povox {

	class Framebuffer;
	class Pipeline;
	class ComputePass;
	class RenderPass;
	struct RenderPassSpecification
	{
		std::string DebugName = "RenderPass";

		Ref<Pipeline> Pipeline = nullptr;

		Ref<RenderPass> PredecessorPass = nullptr;
		Ref<RenderPass> SuccessorPass = nullptr;

		Ref<ComputePass> PredecessorComputePass = nullptr;
		Ref<ComputePass> SuccessorComputePass = nullptr;

		Ref<Framebuffer> TargetFramebuffer = nullptr;
	};

	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void Recreate(uint32_t width, uint32_t height) = 0;

		virtual void BindInput(const std::string& name, Ref<ShaderResource>) = 0;
		virtual void BindOutput(const std::string& name, Ref<ShaderResource>) = 0;

		virtual void UpdateDescriptor(const std::string& name) = 0;

		virtual void Bake() = 0;

		virtual void SetPredecessor(Ref<RenderPass> predecessor) = 0;
		virtual void SetPredecessor(Ref<ComputePass> predecessor) = 0;
		virtual void SetSuccessor(Ref<RenderPass> successor) = 0;
		virtual void SetSuccessor(Ref<ComputePass> successor) = 0;


		virtual const RenderPassSpecification& GetSpecification() const = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};


	class ComputePipeline;
	struct ComputePassSpecification
	{
		std::string DebugName = "ComputePass";

		Ref<ComputePipeline> Pipeline = nullptr;

		Ref<RenderPass> PredecessorPass = nullptr;
		Ref<RenderPass> SuccessorPass = nullptr;

		Ref<ComputePass> PredecessorComputePass = nullptr;
		Ref<ComputePass> SuccessorComputePass = nullptr;

		struct WorkgroupSize
		{
			uint32_t X = 1;
			uint32_t Y = 1;
			uint32_t Z = 1;
		} WorkgroupSize;

		//Ref<Framebuffer> TargetFramebuffer = nullptr;
	};

	class ComputePass
	{
	public:
		virtual ~ComputePass() = default;
		virtual void Recreate() = 0;

		virtual void Bake() = 0;

		virtual void BindInput(const std::string& name, Ref<ShaderResource>) = 0;
		virtual void BindOutput(const std::string& name, Ref<ShaderResource>) = 0;

		virtual void UpdateDescriptor(const std::string& name) = 0;

		virtual void SetPredecessor(Ref<RenderPass> predecessor) = 0;
		virtual void SetPredecessor(Ref<ComputePass> predecessor) = 0;
		virtual void SetSuccessor(Ref<RenderPass> successor) = 0;
		virtual void SetSuccessor(Ref<ComputePass> successor) = 0;

		virtual const ComputePassSpecification& GetSpecification() const = 0;

		//virtual Ref<Image2D> GetFinalImage(uint32_t index) = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<ComputePass> Create(const ComputePassSpecification& spec);
	};
}
