#include "pxpch.h"
#include "VulkanRenderPass.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"

#include "Povox/Core/Log.h"

namespace Povox {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(std::move(spec))
	{
		PX_PROFILE_FUNCTION();

		Recreate();
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}

	void VulkanRenderPass::Recreate()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		if (m_RenderPass)
			vkDestroyRenderPass(device, m_RenderPass, nullptr);

		Ref<VulkanFramebuffer> framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(m_Specification.TargetFramebuffer);
		auto& fbspecs = framebuffer->GetSpecification();


		//TODO: maybe separate depth and color attachments at every step to reduce if calls
		std::vector<VkAttachmentDescription> attachments(fbspecs.Attachments.Attachments.size());
		uint32_t colorCount = 0;
		bool foundDepth = false;
		for (uint32_t i = 0; i < attachments.size(); i++)
		{
			VkAttachmentDescription attachment{};
			VkFormat format = VulkanUtils::GetVulkanImageFormat(fbspecs.Attachments.Attachments[i].Format);
			attachment.format = format;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (VulkanUtils::IsVulkanDepthFormat(format))
			{
				PX_CORE_ASSERT(!foundDepth, "Multiple depthAttachments not allowed!");
				if (!m_Specification.HasDepthAttachment)
					PX_CORE_WARN("VulkanRenderPass::Recreate: Specs.HasDepthAttachment false, but found one!");
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				foundDepth = true;
			}
			else
			{
				if (m_Specification.TargetFramebuffer->GetSpecification().SwapChainTarget)
					attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				else
					attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorCount++;
			}
			attachments[i] = attachment;
		}
		if (!foundDepth && m_Specification.HasDepthAttachment)
			PX_CORE_WARN("VulkanRenderPass::Recreate: Specs.HasDepthAttachment true, but found none!");
		if (colorCount != m_Specification.ColorAttachmentCount)
			PX_CORE_WARN("VulkanRenderPass::Recreate: Specs.ColorAttachmentCount not equal to actual ColorAttachmentCount!");
		m_Specification.ColorAttachmentCount = colorCount;

		std::vector<VkAttachmentReference> colorRefs(colorCount);
		VkAttachmentReference depthref{};
		// TODO: if depth attachment is in the middle of color attachment,s this will break -> ensure depth is always last
		for (uint32_t i = 0; i < attachments.size(); i++)
		{
			if (Utils::IsDepthFormat(fbspecs.Attachments.Attachments[i].Format))
			{
				VkAttachmentReference ref{};
				ref.attachment = i;
				ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				depthref = ref;
			}
			else
			{
				VkAttachmentReference ref{};
				ref.attachment = i;
				ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorRefs[i] = ref;
			}
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();			

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		if (foundDepth && m_Specification.HasDepthAttachment)
		{
			subpass.pDepthStencilAttachment = &depthref;
			dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}


		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		PX_CORE_VK_ASSERT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass), VK_SUCCESS, "Failed to create renderpass!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
		nameInfo.objectHandle = (uint64_t)m_RenderPass;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

		PX_CORE_INFO("VulkanRenderpass::Recreate: Recreated Renderpass with AttachmentExtent of '{0}, {1}'", fbspecs.Width, fbspecs.Height);
		//Now create the actual framebuffer with this render pass
		framebuffer->Construct(m_RenderPass);
	}

	VulkanComputePass::VulkanComputePass(const ComputePassSpecification& spec)
		: m_Specification(spec)
	{
	}

	VulkanComputePass::~VulkanComputePass()
	{
	}

	void VulkanComputePass::Recreate()
	{
	}

}
