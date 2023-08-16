#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Framebuffer.h"
#include "Povox/Renderer/RenderPass.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/Buffer.h"

namespace Povox {

	namespace PipelineUtils {

		enum class PrimitiveTopology
		{
			TRIANGLE_LIST = 3,
			TRIANGLE_STRIP = 4,

			DEFAULT = TRIANGLE_LIST
		};

		enum class PolygonMode
		{
			FILL = 0,
			LINE = 1,
			POINT = 2,

			DEFAULT = FILL
		};

		enum class FrontFace
		{
			COUNTER_CLOCKWISE = 0,
			CLOCKWISE = 1,

			DEFAULT = COUNTER_CLOCKWISE
		};

		enum class CullMode
		{
			NONE = 0,
			FRONT = 1,
			BACK = 2,
			FRONT_AND_BACK = 3,

			DEFAULT = BACK
		};

	}

	
	struct PipelineSpecification
	{
		Ref<Framebuffer> TargetFramebuffer = nullptr;

		Ref<Shader> Shader = nullptr;
		BufferLayout VertexInputLayout;

		PipelineUtils::PrimitiveTopology Primitive = PipelineUtils::PrimitiveTopology::DEFAULT;
		PipelineUtils::PolygonMode FillMode = PipelineUtils::PolygonMode::DEFAULT;
		PipelineUtils::FrontFace Front = PipelineUtils::FrontFace::COUNTER_CLOCKWISE;
		PipelineUtils::CullMode Culling = PipelineUtils::CullMode::DEFAULT;

		bool DepthTesting = true;
		bool DepthWriting = true;


		bool DynamicViewAndScissors = true;
		struct Viewport
		{
			uint32_t X = 0;
			uint32_t Y = 0;
			uint32_t Width = 0;
			uint32_t Height = 0;
			float MinDepth = 0.0f;
			float MaxDepth = 1.0f;
		}Viewport;

		struct Scissor
		{
			uint32_t X = 0;
			uint32_t Y = 0;
			uint32_t Width = 0;
			uint32_t Height = 0;
		}Scissor;

		std::string DebugName = "Pipeline";
	};

	class Buffer;
	class Image2D;
	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;
		virtual void Free() = 0;

		virtual PipelineSpecification& GetSpecification() = 0;

		virtual void Recreate() = 0;

		virtual const std::string& GetDebugName() const = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& specs);
	};


	struct ComputePipelineSpecification
	{
		Ref<Shader> Shader = nullptr;


		std::string DebugName = "ComputePipeline";
	};

	class ComputePipeline
	{
	public:
		virtual ~ComputePipeline() = default;
		virtual void Free() = 0;

		virtual void Recreate() = 0;

		virtual ComputePipelineSpecification& GetSpecification() = 0;
		virtual const std::string& GetDebugName() const = 0;


		static Ref<ComputePipeline> Create(const ComputePipelineSpecification& specs);
	};
}
