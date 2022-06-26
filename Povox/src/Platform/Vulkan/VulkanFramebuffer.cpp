#include "pxpch.h"
#include "VulkanFramebuffer.h"

#include "VulkanContext.h"
#include "Povox/Core/Application.h"
#include "VulkanSwapchain.h"

#include "VulkanDebug.h"


namespace Povox {
	
// --------------- Framebuffer ---------------

	VulkanFramebuffer::VulkanFramebuffer() {}
	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& specs)
		:m_Specification(specs)
	{
		CreateAttachments();
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		Destroy();
	}

	void VulkanFramebuffer::CreateAttachments()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		if (m_Specification.SwapChainTarget && (m_Specification.Attachements.Attachments.size() > 1 || Utils::IsDepthFormat(m_Specification.Attachements.Attachments[0].Format)))
			PX_CORE_ASSERT(true, "Framebuffer as Swapchain target is only allowed to hold ONE COLOR attachment!");

		//First check if images come from elsewhere, if so, use those, if not, create own
		for (auto& attachment : m_Specification.Attachements.Attachments)
		{
			if (!m_Specification.OriginalImages.empty())
			{
				PX_CORE_ASSERT(m_Specification.OriginalImages.size() == m_Specification.Attachements.Attachments.size(), "Framebuffer specification: Original size does not match attachments size!");
				for (uint32_t i = 0; i < m_Specification.OriginalImages.size(); i++)
				{
					if (Utils::IsDepthFormat(m_Specification.Attachements.Attachments[i].Format))
						m_DepthAttachment = m_Specification.OriginalImages[i];
					else
						m_ColorAttachments.push_back(m_Specification.OriginalImages[i]);
				}
			}
			else
			{
				ImageSpecification imageSpec;
				imageSpec.Format = attachment.Format;
				imageSpec.Width = m_Specification.Width * m_Specification.Scale.X;
				imageSpec.Height = m_Specification.Height * m_Specification.Scale.Y;
				imageSpec.Memory = MemoryUsage::GPU_ONLY;
				imageSpec.MipLevels = 1;
				imageSpec.Tiling = ImageTiling::OPTIMAL;
				if (Utils::IsDepthFormat(attachment.Format))
				{
					imageSpec.Usages = { ImageUsage::DEPTH_ATTACHMENT };
					m_DepthAttachment = Image2D::Create(imageSpec);
				}
				else
				{
					imageSpec.Usages = { ImageUsage::COLOR_ATTACHMENT };
					m_ColorAttachments.emplace_back(Image2D::Create(imageSpec));
				}
			}
		}
	}

	//redo!
	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height)
	{	
		PX_PROFILE_FUNCTION();


		if (width == 0 || height == 0)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		//only clear image resources, if this fb owns them
		if (m_Specification.OriginalImages.empty())
		{
			if (m_DepthAttachment)
				m_DepthAttachment->Destroy();
			for (uint32_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				m_ColorAttachments[i]->Destroy();
			}
		}
		if (m_Framebuffer)
		{
			vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
			m_Framebuffer = VK_NULL_HANDLE;
		}

		CreateAttachments();
		Construct();
	}

	void VulkanFramebuffer::Construct()
	{
		if (!m_RenderPass)
		{
			PX_CORE_ASSERT(true, "No Renderpass has been set yet for Framebuffer construction!");
			return;
		}

		std::vector<VkImageView> attachments;
		attachments.resize(m_ColorAttachments.size());
		for (uint32_t i = 0; i < attachments.size(); i++)
		{
			attachments[i] = std::dynamic_pointer_cast<VulkanImage2D>(m_ColorAttachments[i])->GetImageView();
		}
		if (m_DepthAttachment)
			attachments.push_back(std::dynamic_pointer_cast<VulkanImage2D>(m_DepthAttachment)->GetImageView());

		VkFramebufferCreateInfo fbCI{};
		fbCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCI.pNext = nullptr;
		fbCI.width = m_Specification.Width;
		fbCI.height = m_Specification.Height;
		fbCI.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbCI.pAttachments = attachments.data();
		fbCI.renderPass = m_RenderPass;
		PX_CORE_VK_ASSERT(vkCreateFramebuffer(VulkanContext::GetDevice()->GetVulkanDevice(), &fbCI, nullptr, &m_Framebuffer), VK_SUCCESS, "Failed to create Framebuffer!");
	}

	void VulkanFramebuffer::Destroy()
	{
		PX_PROFILE_FUNCTION();


		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		//only clear image resources, if fb own them
		if (m_Specification.OriginalImages.empty())
		{
			if (m_DepthAttachment)
				m_DepthAttachment->Destroy();
			for (uint32_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				m_ColorAttachments[i]->Destroy();
			}
		}
		if (m_Framebuffer)
		{
			vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
			m_Framebuffer = VK_NULL_HANDLE;
		}
		if (m_RenderPass)
		{
			vkDestroyRenderPass(device, m_RenderPass, nullptr); //??? -> maybe just set to null handle, and the actual VUlkanRenderpass takes care of its cleanup
			m_RenderPass = VK_NULL_HANDLE;
		}
	}

	Ref<Image2D> VulkanFramebuffer::GetColorImage(size_t index)
	{
		PX_CORE_ASSERT(index < m_ColorAttachments.size(), "Indext out of range!");
		return m_ColorAttachments[index];
	}
}
