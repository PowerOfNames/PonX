#include "pxpch.h"
#include "VulkanFramebuffer.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanSwapchain.h"

#include "Povox/Core/Application.h"



namespace Povox {
	
// --------------- Framebuffer ---------------

	VulkanFramebuffer::VulkanFramebuffer() {}
	VulkanFramebuffer::VulkanFramebuffer(const FramebufferSpecification& specs)
		:m_Specification(specs)
	{
		CreateAttachments();

		//The actual fb will be constructed AFTER a RP has been created. THis will happen either in FB->create() or manually, when needed using this->Construct(rp)

		PX_CORE_TRACE("VulkanFramebuffer::VulkanFramebuffer: Initialized!");
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		Destroy();
	}

	void VulkanFramebuffer::CreateAttachments()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		if (m_Specification.SwapChainTarget && (m_Specification.Attachments.Attachments.size() > 2 || Utils::IsDepthFormat(m_Specification.Attachments.Attachments[0].Format)))
			PX_CORE_ASSERT(true, "Framebuffer as Swapchain target is only allowed to hold ONE COLOR attachment and ONE DEPTH attachment for now!");
		uint32_t index = 0;
		for (auto& attachment : m_Specification.Attachments.Attachments)
		{
			//First check if images come from elsewhere, if so, use those. If not, create new
			if (!m_Specification.OriginalImages.empty())
			{
				PX_CORE_ASSERT(m_Specification.OriginalImages.size() == m_Specification.Attachments.Attachments.size(), "Framebuffer specification: Original images size does not match attachments size!");
				for (uint32_t i = 0; i < m_Specification.OriginalImages.size(); i++)
				{
					if (Utils::IsDepthFormat(m_Specification.Attachments.Attachments[i].Format))
						m_DepthAttachment = m_Specification.OriginalImages[i];
					else
						m_ColorAttachments.push_back(m_Specification.OriginalImages[i]);
				}
			}
			else
			{
				ImageSpecification imageSpec;
				imageSpec.Format = attachment.Format;
				imageSpec.Width = (uint32_t)((float)m_Specification.Width * m_Specification.Scale.X); //float to int conversion
				imageSpec.Height = (uint32_t)((float)m_Specification.Height * m_Specification.Scale.Y);
				imageSpec.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
				imageSpec.MipLevels = 1;
				imageSpec.Tiling = ImageTiling::OPTIMAL;
				if (Utils::IsDepthFormat(attachment.Format))
				{
					imageSpec.DebugName = m_Specification.DebugName + "-DepthAttachment";
					imageSpec.Usages = { ImageUsage::DEPTH_ATTACHMENT };
					m_DepthAttachment = Image2D::Create(imageSpec);
					std::dynamic_pointer_cast<VulkanImage2D>(m_DepthAttachment)->TransitionImageLayout(
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						0,
						VK_ACCESS_MEMORY_READ_BIT,
						VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					PX_CORE_TRACE("FB: Creating DepthAttachment");
				}
				else
				{
					imageSpec.DebugName = m_Specification.DebugName + "-ColorAttachment " + std::to_string(index);
					imageSpec.Usages = { ImageUsage::COLOR_ATTACHMENT, ImageUsage::COPY_SRC };
					Ref<Image2D> colorImage = Image2D::Create(imageSpec);
					std::dynamic_pointer_cast<VulkanImage2D>(colorImage)->TransitionImageLayout(
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						0,
						VK_ACCESS_MEMORY_READ_BIT,
						VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					m_ColorAttachments.emplace_back(colorImage);
					PX_CORE_TRACE("FB: Creating ColorAttachment");
				}
			}
			index++;
		}
	}

	
	void VulkanFramebuffer::Recreate(uint32_t width, uint32_t height)
	{	
		PX_PROFILE_FUNCTION();


		if (!m_Specification.Resizable)
			return;

		if (width == 0 || height == 0)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		//only clear image resources, if this fb owns them
		if (m_Specification.OriginalImages.empty())
		{
			if (m_DepthAttachment)
			{
				m_DepthAttachment->Free();
				m_DepthAttachment = nullptr;
			}
			for (uint32_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				m_ColorAttachments[i]->Free();
			}
			m_ColorAttachments.clear();
		}
		if (m_Framebuffer)
		{
			vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
			m_Framebuffer = VK_NULL_HANDLE;
		}

		CreateAttachments();
	}

	void VulkanFramebuffer::Construct(VkRenderPass renderpass)
	{
		if (renderpass)
			m_RenderPass = renderpass;

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
		fbCI.layers = 1; //number of layers in images
		PX_CORE_VK_ASSERT(vkCreateFramebuffer(VulkanContext::GetDevice()->GetVulkanDevice(), &fbCI, nullptr, &m_Framebuffer), VK_SUCCESS, "Failed to create Framebuffer!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
		nameInfo.objectHandle = (uint64_t)m_Framebuffer;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

		PX_CORE_INFO("VulkanFramebuffer::Construct: Constructed!");
	}

	void VulkanFramebuffer::Destroy()
	{
		PX_PROFILE_FUNCTION();


		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		//only clear image resources, if fb owns them
		if (m_Specification.OriginalImages.empty())
		{
			for (uint32_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				m_ColorAttachments[i]->Free();
			}
			m_ColorAttachments.clear();
			if (m_DepthAttachment)
			{
				m_DepthAttachment->Free();
				m_DepthAttachment = nullptr;
			}
		}
		if (m_Framebuffer)
		{
			vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
			m_Framebuffer = VK_NULL_HANDLE;
		}
		if (m_RenderPass)
		{
			m_RenderPass = VK_NULL_HANDLE;
		}
	}

	const Ref<Image2D> VulkanFramebuffer::GetColorAttachment(size_t index)
	{
		PX_CORE_ASSERT(index < m_ColorAttachments.size(), "Index out of range!");
		return m_ColorAttachments[index];
	}
}
