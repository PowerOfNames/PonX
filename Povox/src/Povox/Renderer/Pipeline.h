#pragma once
#include "Povox/Core/Core.h"

#include "RenderPass.h"
#include "Shader.h"

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
		Ref<RenderPass> TargetRenderPass = nullptr;
		Ref<Shader> Shader = nullptr;

		PipelineUtils::PrimitiveTopology Primitive = PipelineUtils::PrimitiveTopology::DEFAULT;
		PipelineUtils::PolygonMode FillMode = PipelineUtils::PolygonMode::DEFAULT;
		PipelineUtils::FrontFace Front = PipelineUtils::FrontFace::COUNTER_CLOCKWISE;
		PipelineUtils::CullMode Culling = PipelineUtils::CullMode::DEFAULT;

		bool DepthTesting = true;
		bool DepthWriting = true;


		bool DynamicViewAndScissors = true;

		std::string DebugName = "Pipeline";
	};

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
}
