#include "pxpch.h"
#include "VulkanRenderPass.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanFramebuffer.h"

#include "Povox/Core/Log.h"

namespace Povox {

	VulkanRenderPassBuilder VulkanRenderPassBuilder::Begin()
	{
		VulkanRenderPassBuilder builder;

		return builder;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::AddAttachment(VkAttachmentDescription attachment)
	{
		m_Attachments.push_back(std::move(attachment));
		return *this;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkAttachmentReference>& colorRefs, VkAttachmentReference depthRef)
	{
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();
		subpass.pDepthStencilAttachment = &depthRef;

		m_Subpasses.push_back(std::move(subpass));
		return *this;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkAttachmentReference>& colorRefs)
	{
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();

		m_Subpasses.push_back(std::move(subpass));
		return *this;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::AddDependency(VkSubpassDependency dependency)
	{
		m_Dependencies.push_back(std::move(dependency));
		return *this;
	}

	VkRenderPass VulkanRenderPassBuilder::Build()
	{
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = nullptr;

		renderPassInfo.attachmentCount = static_cast<uint32_t>(m_Attachments.size());
		renderPassInfo.pAttachments = m_Attachments.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(m_Subpasses.size());
		renderPassInfo.pSubpasses = m_Subpasses.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(m_Dependencies.size());
		renderPassInfo.pDependencies = m_Dependencies.data();

		VkRenderPass renderPass;
		PX_CORE_VK_ASSERT(vkCreateRenderPass(VulkanContext::GetDevice()->GetVulkanDevice(), &renderPassInfo, nullptr, &renderPass), VK_SUCCESS, "Failed to create renderpass!");
		return renderPass;
	}


	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(std::move(spec))
	{
		Ref<VulkanFramebuffer> framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(spec.TargetFramebuffer);
		auto& fbspecs = framebuffer->GetSpecification();
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();		


		//TODO: maybe separate depth and color attachments at every step to reduce if calls
		std::vector<VkAttachmentDescription> attachments(fbspecs.Attachments.Attachments.size());
		uint32_t colorCount = 0;
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
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorCount++;
			}
			attachments[i] = attachment;
		}
		std::vector<VkAttachmentReference> colorRefs(colorCount);
		VkAttachmentReference depthref{};
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

		//TODO: subpasses later come from framebuffer chains -> pack multiple renderpasses in one if possible
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();
		subpass.pDepthStencilAttachment = &depthref;

		//From here on out (and also bits of the attachment creation) need to be determined after analysis of the dependencies to other render passes 
		// -> chaining render passes need specific number of images and image formats
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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
		//Now create the actual framebuffer with this render pass
		framebuffer->Construct(m_RenderPass);
	}


	VulkanRenderPass::~VulkanRenderPass()
	{
	}

}
