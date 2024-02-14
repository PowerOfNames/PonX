#include "pxpch.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/Vulkan/VulkanShader.h"

#include "Povox/Renderer/Renderable.h"

#include "glm/glm.hpp"

namespace Povox {

	namespace VulkanUtils {

		static VkPrimitiveTopology GetVulkanPrimitiveTopology(PipelineUtils::PrimitiveTopology topo)
		{
			switch (topo)
			{
				case PipelineUtils::PrimitiveTopology::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				case PipelineUtils::PrimitiveTopology::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

				default: PX_CORE_ASSERT(true, "Topology false or missing!");
			}
		}

		static VkPolygonMode GetVulkanPolygonMode(PipelineUtils::PolygonMode mode)
		{
			switch (mode)
			{
				case PipelineUtils::PolygonMode::FILL: return VK_POLYGON_MODE_FILL;
				case PipelineUtils::PolygonMode::LINE: return VK_POLYGON_MODE_LINE;
				case PipelineUtils::PolygonMode::POINT: return VK_POLYGON_MODE_POINT;

				default: PX_CORE_ASSERT(true, "PolygonMode false or missing!");
			}
		}

		static VkFrontFace GetVulkanFrontFace(PipelineUtils::FrontFace face)
		{
			switch (face)
			{
				case PipelineUtils::FrontFace::COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
				case PipelineUtils::FrontFace::CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;

				default: PX_CORE_ASSERT(true, "FrontFace false!");
			}
		}

		static VkCullModeFlagBits GetVulkanCullMode(PipelineUtils::CullMode culling)
		{
			switch (culling)
			{
				case PipelineUtils::CullMode::NONE: return VK_CULL_MODE_NONE;
				case PipelineUtils::CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
				case PipelineUtils::CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
				case PipelineUtils::CullMode::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;

				default: PX_CORE_ASSERT(true, "CullMode false!");
			}
		}
	}

	struct VulkanVertexData
	{
		QuadVertex VertexProperties;

		static VertexInputDescription GetVertexDescription()
		{
			VertexInputDescription output;

			VkVertexInputBindingDescription vertexBindingDescription{};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = sizeof(VulkanVertexData);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			output.Bindings.push_back(vertexBindingDescription);

			VkVertexInputAttributeDescription positionAttribute{};
			positionAttribute.binding = 0;
			positionAttribute.location = 0;
			positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			positionAttribute.offset = offsetof(VulkanVertexData, VertexProperties.Position);

			VkVertexInputAttributeDescription colorAttribute{};
			colorAttribute.binding = 0;
			colorAttribute.location = 1;
			colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttribute.offset = offsetof(VulkanVertexData, VertexProperties.Color);

			VkVertexInputAttributeDescription uvAttributes{};
			uvAttributes.binding = 0;
			uvAttributes.location = 2;
			uvAttributes.format = VK_FORMAT_R32G32_SFLOAT;
			uvAttributes.offset = offsetof(VulkanVertexData, VertexProperties.TexCoord);

			output.Attributes.push_back(positionAttribute);
			output.Attributes.push_back(colorAttribute);
			output.Attributes.push_back(uvAttributes);

			return output;
		}
	};

	VulkanPipeline::VulkanPipeline(const PipelineSpecification& specs)
		:m_Specification(specs)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_ASSERT(m_Specification.TargetFramebuffer, "There was no Framebuffer attached to this Pipeline!");
		CreateLayout();	
		CreatePipeline();
	}
	VulkanPipeline::~VulkanPipeline() {}

	void VulkanPipeline::Free()
	{
		VulkanContext::SubmitResourceFree([=]() {
			VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
			if (m_Layout)
			{
				vkDestroyPipelineLayout(device, m_Layout, nullptr);
				m_Layout = VK_NULL_HANDLE;
			}
			if (m_Pipeline)
			{
				vkDestroyPipeline(device, m_Pipeline, nullptr);
				m_Pipeline = VK_NULL_HANDLE;
			}
		});

	}

	void VulkanPipeline::Recreate()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		if (m_Specification.DynamicViewAndScissors)
			return;
		
		if (m_Pipeline)
		{
			vkDestroyPipeline(device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		//Check if I need to recreate the layout as well!
		CreatePipeline();
	}

	void VulkanPipeline::CreateLayout()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Shader);

		auto& layoutMap = shader->GetDescriptorSetLayouts();
		std::vector<VkDescriptorSetLayout> layouts;
		for (const auto& [setNumber, layout] : layoutMap)
		{
			layouts.push_back(layout);
		}


		PX_CORE_WARN("Layouts: {0}", layouts.size());
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;

		layoutInfo.pushConstantRangeCount = 0;		// optional
		layoutInfo.pPushConstantRanges = nullptr;	// optional
		layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		PX_CORE_INFO("DescriptorSetLayout count = '{0}'", layoutInfo.setLayoutCount);
		layoutInfo.pSetLayouts = layouts.data();

		VkPushConstantRange pushConstant;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(PushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;
		PX_CORE_VK_ASSERT(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_Layout), VK_SUCCESS, "Failed to create GraphicsPipelineLayout!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		nameInfo.objectHandle = (uint64_t)m_Layout;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

		PX_CORE_TRACE("VulkanPipeline::Construct Created PipelineLayout!");
	}


	void VulkanPipeline::CreatePipeline()
	{
		PX_CORE_ASSERT(m_Specification.TargetFramebuffer, "Framebuffer missing!");


		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Shader);
		Ref<VulkanFramebuffer> framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);

		{
			VkPipelineVertexInputStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			//VertexInputDescription comes from shader-reflect -> should be validated with m_Specification.Layout
			VertexInputDescription& description = shader->GetVertexInputDescription();
			info.vertexBindingDescriptionCount = static_cast<uint32_t>(description.Bindings.size());
			info.pVertexBindingDescriptions = description.Bindings.data();
			info.vertexAttributeDescriptionCount = static_cast<uint32_t>(description.Attributes.size());
			info.pVertexAttributeDescriptions = description.Attributes.data();
			m_VertexInputStateInfo = info;
		}

		for (auto& [stage, module] : shader->GetModules())
		{
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;

			info.stage = stage;
			info.module = module;
			info.pName = "main";
			m_ShaderStageInfos.push_back(info);
		}
		{
			VkPipelineInputAssemblyStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.topology = VulkanUtils::GetVulkanPrimitiveTopology(m_Specification.Primitive);
			info.primitiveRestartEnable = VK_FALSE;
			m_AssemblyStateInfo = info;
		}
		{
			m_Scissor.extent.width = m_Specification.Scissor.Width;
			m_Scissor.extent.height = m_Specification.Scissor.Height;
			m_Scissor.offset.x = m_Specification.Scissor.X;
			m_Scissor.offset.y = m_Specification.Scissor.Y;

			m_Viewport.x = m_Specification.Viewport.X;
			m_Viewport.y = m_Specification.Viewport.Y;
			m_Viewport.minDepth = m_Specification.Viewport.MinDepth;
			m_Viewport.maxDepth = m_Specification.Viewport.MaxDepth;
		}
		{
			VkPipelineRasterizationStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthClampEnable = VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
			info.rasterizerDiscardEnable = VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
			info.polygonMode = VulkanUtils::GetVulkanPolygonMode(m_Specification.FillMode);					// determines if filled, lines or point shall be rendered
			info.lineWidth = 1.0f;									// thicker then 1.0f requires wideLine GPU feature
			info.cullMode = VulkanUtils::GetVulkanCullMode(m_Specification.Culling);
			info.frontFace = VulkanUtils::GetVulkanFrontFace(m_Specification.Front);
			info.depthBiasEnable = VK_FALSE;
			m_RasterizationStateInfo = info;
		}
		{
			VkPipelineMultisampleStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.sampleShadingEnable = VK_FALSE;
			info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			info.minSampleShading = 1.0f;		//	optional
			info.pSampleMask = nullptr;	//	optional
			info.alphaToCoverageEnable = VK_FALSE;	//	optional
			info.alphaToOneEnable = VK_FALSE;	//	optional
			m_MultisampleStateInfo = info;
		}
		{
			VkPipelineDepthStencilStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.depthTestEnable = m_Specification.DepthTesting;
			info.depthWriteEnable = m_Specification.DepthWriting;
			info.depthCompareOp = VK_COMPARE_OP_LESS;
			info.depthBoundsTestEnable = VK_FALSE;
			info.minDepthBounds = 0.0f;		// optional
			info.maxDepthBounds = 1.0f;		// optional
			info.stencilTestEnable = VK_FALSE;
			info.front = {};
			info.back = {};
			m_DepthStencilStateInfo = info;
		}
		{
			VkPipelineColorBlendAttachmentState info{};
			info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			info.blendEnable = VK_TRUE;
			info.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			info.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			info.colorBlendOp = VK_BLEND_OP_ADD;
			info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			info.alphaBlendOp = VK_BLEND_OP_ADD;
			/* in pseudo code, this happens while blending
			if (blendEnable)
				finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
				finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
			else
				finalColor = newColor;
			finalColor = finalColor & colorWriteMask;
			*/
			//check http://andersriggelsen.dk/glblendfunc.php for testing and understanding more!


			m_ColorBlendAttachmentStateInfos.push_back(info);
			//TODO: work out how this works when writing to multiple attachments (color, identifier...) //can supposedly be done via the Attachment_Format -> some do not support blending
			//check, if current device supports independent blending, if not -> create separate pipeline for the attachment ?!
			info.blendEnable = VK_FALSE;
			m_ColorBlendAttachmentStateInfos.push_back(info);
		}

		{
			VkPipelineViewportStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			info.pNext = nullptr;

			info.viewportCount = 1;
			info.pViewports = &m_Viewport;
			info.scissorCount = 1;
			info.pScissors = &m_Scissor;
			m_ViewportStateInfo = info;
		}

		{
			VkPipelineColorBlendStateCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			info.logicOpEnable = VK_FALSE;
			info.logicOp = VK_LOGIC_OP_COPY;
			info.attachmentCount = m_Specification.TargetFramebuffer->GetSpecification().ColorAttachmentCount;
			info.pAttachments = m_ColorBlendAttachmentStateInfos.data();
			info.blendConstants[0] = 0.0f;
			info.blendConstants[1] = 0.0f;
			info.blendConstants[2] = 0.0f;
			info.blendConstants[3] = 0.0f;
			m_ColorBlendStateInfo = info;
		}

		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		if (m_Specification.DynamicViewAndScissors)
		{
			dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
		}
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();

		// The Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = static_cast<uint32_t>(m_ShaderStageInfos.size());
		pipelineInfo.pStages = m_ShaderStageInfos.data();
		pipelineInfo.pVertexInputState = &m_VertexInputStateInfo;
		pipelineInfo.pInputAssemblyState = &m_AssemblyStateInfo;
		pipelineInfo.pRasterizationState = &m_RasterizationStateInfo;
		pipelineInfo.pMultisampleState = &m_MultisampleStateInfo;
		pipelineInfo.pDepthStencilState = &m_DepthStencilStateInfo;
		pipelineInfo.pColorBlendState = &m_ColorBlendStateInfo;
		pipelineInfo.pViewportState = &m_ViewportStateInfo;
		if (m_Specification.DynamicViewAndScissors)
			pipelineInfo.pDynamicState = &dynamicStateInfo;
		else
			pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = framebuffer->GetRenderPass();
		pipelineInfo.subpass = 0; // index

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT is specified under flags in VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineIndex = -1;


		PX_CORE_VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), VK_SUCCESS, "Failed to create Graphics pipeline!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		nameInfo.objectHandle = (uint64_t)m_Pipeline;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG
	}

	void VulkanPipeline::PrintShaderLayout() const
	{
		auto& shaderLayout = m_Specification.Shader->GetResourceDescriptions();
		for (auto&& [name, binding] : shaderLayout)
		{
			PX_CORE_TRACE("Pipeline: {}", m_Specification.DebugName);
			PX_CORE_TRACE("Shader: {}", m_Specification.Shader->GetDebugName());
			PX_CORE_INFO("BindingName: {}", name, binding->Name);
			PX_CORE_INFO("Set {}, Binding {}, Type: {}", binding->Set, binding->Binding, ToStringUtility::ShaderResourceTypeToString(binding->ResourceType));
			PX_CORE_INFO("{}", ToStringUtility::ShaderStageToString(binding->Stages));
		}
	}

	//------------------ Compute -------------------------


	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineSpecification& specs)
		: m_Specification(specs)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("VulkanComputePipeline::Construct Begin!");
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(specs.Shader);

		CreateLayout();
		CreatePipeline();

		PX_CORE_TRACE("VulkanComputePipeline::Construct Finished!");
	}
	VulkanComputePipeline::~VulkanComputePipeline()	{}

	void VulkanComputePipeline::Free()
	{
		VulkanContext::SubmitResourceFree([=]() {

			VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
			if (m_Layout)
			{
				vkDestroyPipelineLayout(device, m_Layout, nullptr);
				m_Layout = VK_NULL_HANDLE;
			}
			if (m_Pipeline)
			{
				vkDestroyPipeline(device, m_Pipeline, nullptr);
				m_Pipeline = VK_NULL_HANDLE;
			}
			});
	}

	void VulkanComputePipeline::Recreate()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		//if (m_Specification.DynamicViewAndScissors)
		//	return;

		if (m_Pipeline)
			vkDestroyPipeline(device, m_Pipeline, nullptr);
		CreatePipeline();
	}

	void VulkanComputePipeline::CreateLayout()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Shader);

		auto& layoutMap = shader->GetDescriptorSetLayouts();
		std::vector<VkDescriptorSetLayout> layouts;
		for (const auto& [setNumber, layout] : layoutMap)
		{
			layouts.push_back(layout);
		}
		PX_CORE_WARN("Layouts: {0}", layouts.size());
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;

		//layoutInfo.pushConstantRangeCount = 0;		// optional
		//layoutInfo.pPushConstantRanges = nullptr;	// optional
		layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		PX_CORE_INFO("DescriptorSetLayout count = '{0}'", layoutInfo.setLayoutCount);
		layoutInfo.pSetLayouts = layouts.data();

// 		VkPushConstantRange pushConstant;
// 		pushConstant.offset = 0;
// 		pushConstant.size = sizeof(PushConstants);
// 		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
// 
// 		layoutInfo.pushConstantRangeCount = 1;
// 		layoutInfo.pPushConstantRanges = &pushConstant;
		PX_CORE_VK_ASSERT(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_Layout), VK_SUCCESS, "Failed to create ComputePipelineLayout!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		nameInfo.objectHandle = (uint64_t)m_Layout;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

		PX_CORE_TRACE("VulkanPipeline::Construct Created ComputePipelineLayout!");
	}

	void VulkanComputePipeline::CreatePipeline()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		Ref<VulkanShader> shader = std::dynamic_pointer_cast<VulkanShader>(m_Specification.Shader);

		//TODO: Check what modules exist and iterate over them
		const std::unordered_map<VkShaderStageFlagBits, VkShaderModule> modules = shader->GetModules();
		if (modules.find(VK_SHADER_STAGE_COMPUTE_BIT) == modules.end())
		{
			PX_CORE_ASSERT(true, "There is no compute shader set in this PipelineConfig!");
		}
		else
		{
			VkPipelineShaderStageCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.pNext = nullptr;

			info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			info.module = modules.at(VK_SHADER_STAGE_COMPUTE_BIT);
			info.pName = "main";
			m_ShaderStageInfo = info;
		}

		VkComputePipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.pNext = nullptr;

		info.layout = m_Layout;
		info.stage = m_ShaderStageInfo;

		info.flags = 0;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = 0;

		PX_CORE_VK_ASSERT(vkCreateComputePipelines(device, nullptr, 1, &info, nullptr, &m_Pipeline), VK_SUCCESS, "Failed to create ComputePipeline!");

#ifdef PX_DEBUG
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
		nameInfo.objectHandle = (uint64_t)m_Pipeline;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG
	}


	void VulkanComputePipeline::PrintShaderLayout() const
	{
		auto& shaderLayout = m_Specification.Shader->GetResourceDescriptions();
		for (auto&& [name, binding] : shaderLayout)
		{
			PX_CORE_TRACE("Pipeline: {}", m_Specification.DebugName);
			PX_CORE_TRACE("Shader: {}", m_Specification.Shader->GetDebugName());
			PX_CORE_INFO("BindingName: {}", name, binding->Name);
			PX_CORE_INFO("Set {}, Binding {}, Type: {}", binding->Set, binding->Binding, ToStringUtility::ShaderResourceTypeToString(binding->ResourceType));
			PX_CORE_INFO("{}", ToStringUtility::ShaderStageToString(binding->Stages));
		}
	}
}
