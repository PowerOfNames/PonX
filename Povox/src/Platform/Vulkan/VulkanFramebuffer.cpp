#include "pxpch.h"
#include "VulkanFramebuffer.h"

#include "VulkanDebug.h"


namespace Povox {

// --------------- Framebuffer ---------------
	VulkanFramebuffer::VulkanFramebuffer() {}
	VulkanFramebuffer::VulkanFramebuffer(VulkanCoreObjects* coreObjects, uint32_t width, uint32_t height)
		: m_Core(coreObjects), m_Width(width), m_Height(height)
	{}

	void VulkanFramebuffer::Create(VkRenderPass renderPass)
	{
		m_RenderPass = renderPass;
		VkFramebufferCreateInfo framebufferInfo = VulkanInits::CreateFramebufferInfo(renderPass, m_Width, m_Height);
		std::vector<VkImageView> attachmentViews;
		attachmentViews.resize(m_ColorAttachments.size());
		for(size_t i = 0; i < m_ColorAttachments.size(); i++)
		{
			attachmentViews[i] = m_ColorAttachments[i].ImageView;
		}
		if (m_HasDepthStencil)
			attachmentViews.push_back(m_DepthStencilAttachment.ImageView);
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());					// The attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound to the respective attachment descriptions in the render pass pAttachment array.
		framebufferInfo.pAttachments = attachmentViews.data();
		PX_CORE_VK_ASSERT(vkCreateFramebuffer(m_Core->Device, &framebufferInfo, nullptr, &m_Framebuffer), VK_SUCCESS, "Failed to create framebuffer!");
		m_Alive = true;
	}

	void VulkanFramebuffer::Destroy()
	{
		PX_CORE_WARN("Destroying framebuffer!");
		vkDestroyFramebuffer(m_Core->Device, m_Framebuffer, nullptr);
		for (auto& attachment : m_ColorAttachments)
		{
			vkDestroyImageView(m_Core->Device, attachment.ImageView, nullptr);
			vmaDestroyImage(m_Core->Allocator, attachment.Image.Image, attachment.Image.Allocation);
		}
		m_ColorAttachments.clear();
		vkDestroyImageView(m_Core->Device, m_DepthStencilAttachment.ImageView, nullptr);
		vmaDestroyImage(m_Core->Allocator, m_DepthStencilAttachment.Image.Image, m_DepthStencilAttachment.Image.Allocation);
		m_HasDepthStencil = false;
		m_Alive = false;
	}

	void VulkanFramebuffer::Resize(VkRenderPass renderPass, uint32_t width, uint32_t height)
	{	
		PX_CORE_WARN("Resizing framebuffer!");
		if(m_Alive == true)
			Destroy();
		m_Width = width;
		m_Height = height;
		for (uint32_t i = 0; i < m_CreateInfos.size(); i++)
		{
			m_CreateInfos[i].Width = width;
			m_CreateInfos[i].Height = height;
			CreateAttachment(i);
		}
		m_RenderPass = renderPass;
		Create(m_RenderPass);
	}

	void VulkanFramebuffer::AddAttachment(FramebufferAttachmentCreateInfo& attachmentCreateInfo)
	{
		m_CreateInfos.push_back(attachmentCreateInfo);
		CreateAttachment(m_CreateInfos.size() - 1);
	}

	void VulkanFramebuffer::CreateAttachment(uint32_t index)
	{
		FramebufferAttachmentCreateInfo& info = m_CreateInfos[index];
		FramebufferAttachment attachment;
		attachment.Width = info.Width;
		attachment.Height = info.Height;
		attachment.Format = info.Format;

		attachment.Image = VulkanImage::Create(*m_Core, attachment.Width, attachment.Height, attachment.Format, VK_IMAGE_TILING_OPTIMAL, info.ImageUsage, info.MemoryUsage);

		VkImageAspectFlags aspect = VulkanUtils::IsDepthAttachment(attachment.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		VkImageViewCreateInfo ivInfo = VulkanInits::CreateImageViewInfo(attachment.Format, attachment.Image.Image, aspect);
		vkCreateImageView(m_Core->Device, &ivInfo, nullptr, &attachment.ImageView);

		if (attachment.HasDepthStencil())
		{
			PX_ASSERT(!m_HasDepthStencil, "Only one depth/stencil attachment allowed!");
			m_DepthStencilAttachment = attachment;
			m_HasDepthStencil = true;
		}
		else
			m_ColorAttachments.push_back(attachment);
	}

	VkRenderPass VulkanFramebuffer::CreateDefaultRenderpass()
	{
		VulkanRenderPassBuilder builder = VulkanRenderPassBuilder::Begin(m_Core);
		for (auto& attachment : m_ColorAttachments)
		{
			builder.AddAttachment(attachment.Description);
		}
		if (m_DepthStencilAttachment.Format & VulkanUtils::FindDepthFormat(m_Core->PhysicalDevice))
			builder.AddAttachment(m_DepthStencilAttachment.Description);

		std::vector<VkImageLayout> colorRefLayouts;
		colorRefLayouts.resize(m_ColorAttachments.size());
		for (size_t i = 0; i < colorRefLayouts.size(); i++)
		{
			colorRefLayouts[i] = m_ColorAttachments[i].Description.finalLayout;
		}
		if (m_DepthStencilAttachment.Format & VulkanUtils::FindDepthFormat(m_Core->PhysicalDevice))
			builder.CreateAndAddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colorRefLayouts, m_DepthStencilAttachment.Description.initialLayout);
		else
			builder.CreateAndAddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, colorRefLayouts);
		return builder.Build();
	}

// --------------- RenderPass ---------------
	VulkanRenderPassBuilder VulkanRenderPassBuilder::Begin(VulkanCoreObjects* core)
	{
		VulkanRenderPassBuilder builder;
		builder.m_Core = core;

		return builder;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::AddAttachment(VkAttachmentDescription attachment)
	{
		m_Attachments.push_back(std::move(attachment));
		return *this;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::CreateAndAddSubpass(VkPipelineBindPoint pipelineBindpoint, const std::vector<VkImageLayout>& colorLayouts, VkImageLayout depthLayout)
	{
		std::vector<VkAttachmentReference> colorRefs;
		colorRefs.resize(colorLayouts.size());
		size_t i;
		for (i = 1; i <= colorLayouts.size(); i++)
		{
			colorRefs[i].attachment = i;
			colorRefs[i].layout = colorLayouts[i];
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();
		if (depthLayout)
		{
			VkAttachmentReference depth{};
			depth.attachment = i;
			depth.layout = depthLayout;
			subpass.pDepthStencilAttachment = &depth;
		}

		m_Subpasses.push_back(std::move(subpass));
		return *this;
	}

	VulkanRenderPassBuilder& VulkanRenderPassBuilder::CreateAndAddDependency(VkSubpassDependency dependency)
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
		PX_CORE_VK_ASSERT(vkCreateRenderPass(m_Core->Device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS, "Failed to create renderpass!");
		return renderPass;
	}
}
