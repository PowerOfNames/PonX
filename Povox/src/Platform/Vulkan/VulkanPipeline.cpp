#include "pxpch.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanDebug.h"

#include "glm/glm.hpp"

namespace Povox {

//------------------RenderPass----------------------
	void VulkanRenderPass::Create(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkFormat swapchainImageFormat)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;		// format of swapchain images
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;	// load and store ops determine what to do with the data before rendering (apply to depth data)
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// we want the image to be ready for presentation in the swapchain, so final is present layout
		// Layout is importent to know, because the next operation the image is involved in needs that
		// initial is before render pass, final is the layout the imiga is automatically transitioned to after the render pass finishes

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VulkanUtils::FindDepthFormat(physicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;		// index in attachment description array
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;		// index in attachment description array
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Subpasses  -> usefull for example to create a sequence of post processing effects
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{}; // dependencies between subpasses
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;	// refers to before or after the render pass depending on it being specified on src or dst
		dependency.dstSubpass = 0;						// index of subpass, dstSubpass must always be higher then src subpass unless one of them is VK_SUBPASS_EXTERNAL
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // operation to wait on
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment }; // attachment description array
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;


		VkRenderPass renderPass;
		PX_CORE_VK_ASSERT(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS, "Failed to create render pass!");
		m_RenderPass = renderPass;
	}

	void VulkanRenderPass::Destroy(VkDevice logicalDevice)
	{
		vkDestroyRenderPass(logicalDevice, m_RenderPass, nullptr);
	}

//------------------RenderPipeline----------------------
	void VulkanPipeline::Create(VkDevice logicalDevice, VkExtent2D swapchainExtent, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout)
	{
		VkShaderModule vertexShaderModule = VulkanShader::Create(logicalDevice, "assets/shaders/vert.spv");
		VkShaderModule fragmentShaderModule = VulkanShader::Create(logicalDevice, "assets/shaders/frag.spv");

		VkPipelineShaderStageCreateInfo vertexShaderInfo{};
		vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderInfo.module = vertexShaderModule;
		vertexShaderInfo.pName = "main";						//entry point

		VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
		fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderInfo.module = fragmentShaderModule;
		fragmentShaderInfo.pName = "main";						//entry point

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderInfo, fragmentShaderInfo };


		// Describes Vertex data layout
		auto bindingDescription = VertexData::GetBindingDescription();
		auto attributeDescriptions = VertexData::GetAttributeDescriptions();


		VkPipelineVertexInputStateCreateInfo vertexStateInfo{};
		vertexStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexStateInfo.vertexBindingDescriptionCount = 1;
		vertexStateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexStateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexStateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


		// Kind of geometry and (primitive restart?)
		VkPipelineInputAssemblyStateCreateInfo assemblyStateInfo{};
		assemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assemblyStateInfo.primitiveRestartEnable = VK_FALSE;


		// Viewports and scissors
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainExtent.width;
		viewport.height = (float)swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;

		VkPipelineViewportStateCreateInfo viewportStateInfo{};
		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.pViewports = &viewport;
		viewportStateInfo.scissorCount = 1;
		viewportStateInfo.pScissors = &scissor;


		// Rasterizer -> rasterization, face culling, depth testing, scissoring, wire frame rendering
		VkPipelineRasterizationStateCreateInfo rasterizationStateInfo{};
		rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateInfo.depthClampEnable = VK_FALSE;								// if true, too near or too far fragments will be clamped instead of discareded
		rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;								// if true, geometry will never be rasterized, so there will be no output to the framebuffer
		rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;					// determines if filled, lines or point shall be rendered
		rasterizationStateInfo.lineWidth = 1.0f;									// thicker then 1.0f reuires wideLine GPU feature
		rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateInfo.depthBiasEnable = VK_FALSE;


		// Multisampling   -> one way to anti-aliasing			(enabling also requires a GPU feature)
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.minSampleShading = 1.0f;		//	optional
		multisampleInfo.pSampleMask = nullptr;	//	optional
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;	//	optional
		multisampleInfo.alphaToOneEnable = VK_FALSE;	//	optional


		// Depth buffer and stencil testing
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = VK_TRUE;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.minDepthBounds = 0.0f;		// optional
		depthStencilInfo.maxDepthBounds = 1.0f;		// optional
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = {};
		depthStencilInfo.back = {};


		//Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // conficuration per attached framebuffer
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;		// optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;			// optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;		// optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;			// optional

		/* in pseudo code, this happens while blending
		if (blendEnable)
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
		else
			finalColor = newColor;
		finalColor = finalColor & colorWriteMask;
		*/

		VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
		colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateInfo.attachmentCount = 1;
		colorBlendStateInfo.pAttachments = &colorBlendAttachment;
		colorBlendStateInfo.blendConstants[0] = 0.0f;
		colorBlendStateInfo.blendConstants[1] = 0.0f;
		colorBlendStateInfo.blendConstants[2] = 0.0f;
		colorBlendStateInfo.blendConstants[3] = 0.0f;


		// Dynamic state  -> allows to modify some of the states above during runtime without recreating the pipeline, such as blend constants, line width and viewport
		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo{}; // with no dynamic state, just pass in a nullptr later on
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = 2;
		dynamicStateInfo.pDynamicStates = dynamicStates;


		// Pipeline layouts  -> commonly used to pass transformation matrices or texture samplers to the shader
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 1;		// optional
		layoutInfo.pSetLayouts = &descriptorSetLayout;	// optional
		layoutInfo.pushConstantRangeCount = 0;		// optional
		layoutInfo.pPushConstantRanges = nullptr;	// optional


		PX_CORE_VK_ASSERT(vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &m_Layout), VK_SUCCESS, "Failed to create pipeline layout!");


		// The Pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexStateInfo;
		pipelineInfo.pInputAssemblyState = &assemblyStateInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizationStateInfo;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		pipelineInfo.pDepthStencilState = &depthStencilInfo;
		pipelineInfo.pColorBlendState = &colorBlendStateInfo;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0; // index

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;	// only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT is specified under flags in VkGraphicsPipelineCreateInfo
		pipelineInfo.basePipelineIndex = -1;


		PX_CORE_VK_ASSERT(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), VK_SUCCESS, "Failed to create Graphics pipeline!");

		vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
		vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
	}

	void VulkanPipeline::Destroy(VkDevice logicalDevice)
	{
		vkDestroyPipeline(logicalDevice, m_Pipeline, nullptr);
	}

	void VulkanPipeline::DestroyLayout(VkDevice logicalDevice)
	{
		vkDestroyPipelineLayout(logicalDevice, m_Layout, nullptr);
	}

}
