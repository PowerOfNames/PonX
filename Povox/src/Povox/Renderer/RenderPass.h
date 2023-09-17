#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Utils/ShaderResources.h"

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

	class UniformBuffer;
	class StorageBuffer;
	class StorageImage;
	class Image2D;
	class RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void Recreate(uint32_t width, uint32_t height) = 0;

		virtual void BindResource(const std::string& name, Ref<UniformBuffer> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<StorageBuffer> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<StorageImage> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<Image2D> resource) = 0;

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
	};

	class ComputePass
	{
	public:
		virtual ~ComputePass() = default;
		virtual void Recreate() = 0;

		virtual void BindResource(const std::string& name, Ref<UniformBuffer> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<StorageBuffer> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<StorageImage> resource) = 0;
		virtual void BindResource(const std::string& name, Ref<Image2D> resource) = 0;

		virtual void Bake() = 0;

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
