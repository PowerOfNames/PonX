#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Utils/ShaderResource.h"

namespace Povox {

	enum PassType
	{
		UNDEFINED,
		GRAPHICS,
		COMPUTE
	};	

	class GPUPass
	{
	public:
		virtual ~GPUPass() = default;

		virtual void Recreate(uint32_t width, uint32_t height) = 0;

		virtual void BindInput(const std::string& name, Ref<ShaderResource>) = 0;
		virtual void BindOutput(const std::string& name, Ref<ShaderResource>) = 0;

		virtual void UpdateDescriptor(const std::string& name) = 0;

		virtual void Bake() = 0;

		virtual void SetPredecessor(Ref<GPUPass> predecessor) = 0;
		virtual void SetSuccessor(Ref<GPUPass> successor) = 0;

		virtual const std::string& GetDebugName() const = 0;

		virtual PassType GetPassType() = 0;
	};

	class Framebuffer;
	class Pipeline;
	struct RenderPassSpecification
	{
		Ref<Pipeline> Pipeline = nullptr;		
		Ref<Framebuffer> TargetFramebuffer = nullptr;

		bool DoPerformanceQuery = false;
		std::string DebugName = "RenderPass";
	};

	class RenderPass : public virtual GPUPass
	{
	public:
		virtual ~RenderPass() = default;

		virtual const RenderPassSpecification& GetSpecification() const = 0;
		virtual RenderPassSpecification& GetSpecification() = 0;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
	};

	class ComputePipeline;
	struct ComputePassSpecification
	{
		std::string DebugName = "ComputePass";
		bool DoPerformanceQuery = false;

		Ref<ComputePipeline> Pipeline = nullptr;
		
		struct LocalWorkgroupSize
		{
			uint32_t X = 1;
			uint32_t Y = 1;
			uint32_t Z = 1;
		} LocalWorkgroupSize;

		//Ref<Framebuffer> TargetFramebuffer = nullptr;
	};

	class ComputePass : public virtual GPUPass
	{
	public:
		virtual ~ComputePass() = default;

		virtual const ComputePassSpecification& GetSpecification() const = 0;
		virtual ComputePassSpecification& GetSpecification() = 0;

		//virtual Ref<Image2D> GetFinalImage(uint32_t index) = 0;

		static Ref<ComputePass> Create(const ComputePassSpecification& spec);
	};
}
