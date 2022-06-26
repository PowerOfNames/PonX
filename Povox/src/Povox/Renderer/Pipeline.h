#pragma once
#include "Povox/Core/Core.h"

#include "RenderPass.h"
#include "Shader.h"

namespace Povox {

	struct PipelineSpecification
	{
		Ref<RenderPass> TargetRenderPass = nullptr;
		Ref<Shader> Shader = nullptr;
		bool DynamicViewAndScissors = true;
	};

	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;
		virtual PipelineSpecification& GetSpecification() = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& specs);
	};
}
