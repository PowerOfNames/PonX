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
		CreateRenderPass();
		CreateFramebuffer();

		PX_CORE_TRACE("VulkanFramebuffer::VulkanFramebuffer: Initialized!");
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		Destroy();
	}

	void VulkanFramebuffer::CreateAttachments()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		uint32_t index = 0;
		m_Specification.HasDepthAttachment = false;
		//First check if images come from elsewhere, if so, use those. If not, create new
		if (!m_Specification.OriginalImages.empty())
		{
			PX_CORE_ASSERT(m_Specification.OriginalImages.size() == m_Specification.Attachments.Attachments.size(), "Framebuffer specification: Original images size does not match attachments size!");
			for (uint32_t i = 0; i < m_Specification.OriginalImages.size(); i++)
			{
				if (Utils::IsDepthFormat(m_Specification.Attachments.Attachments[i].Format))
				{
					PX_CORE_ASSERT(!m_Specification.HasDepthAttachment, "VulkanFramebuffer: Only one Depth-Atachment allowed!");

					m_DepthAttachment = m_Specification.OriginalImages[i];
					m_Specification.HasDepthAttachment = true;
				}
				else
				{
					m_ColorAttachments.push_back(m_Specification.OriginalImages[i]);
					m_Specification.ColorAttachmentCount++;
				}
			}
		}
		else
		{
			for (auto& attachment : m_Specification.Attachments.Attachments)
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
					PX_CORE_ASSERT(!m_Specification.HasDepthAttachment, "VulkanFramebuffer: Only one Depth-Atachment allowed!");

					imageSpec.DebugName = m_Specification.DebugName + "-DepthAttachment";
					imageSpec.Usages = { ImageUsage::DEPTH_ATTACHMENT };
					m_DepthAttachment = Image2D::Create(imageSpec);
					std::dynamic_pointer_cast<VulkanImage2D>(m_DepthAttachment)->TransitionImageLayout(
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT
					);
					PX_CORE_TRACE("FB: Creating DepthAttachment");
					m_Specification.HasDepthAttachment = true;
				}
				else
				{
					imageSpec.DebugName = m_Specification.DebugName + "-ColorAttachment " + std::to_string(index);
					imageSpec.Usages = { ImageUsage::COLOR_ATTACHMENT, ImageUsage::COPY_SRC };
					Ref<Image2D> colorImage = Image2D::Create(imageSpec);
					std::dynamic_pointer_cast<VulkanImage2D>(colorImage)->TransitionImageLayout(
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_MEMORY_READ_BIT
					);
					m_ColorAttachments.emplace_back(colorImage);
					m_Specification.ColorAttachmentCount++;
					PX_CORE_TRACE("FB: Creating ColorAttachment");
				}
			}
			index++;
		}
	}

	// TODO: Refactor this! The cold is copyied from RenderPass construction. VkRenderpass instance is now constructed here and passed over to the VulkanRenderPass class during its Create()  
	void VulkanFramebuffer::CreateRenderPass()
	{

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		//TODO: maybe separate depth and color attachments at every step to reduce if calls
		std::vector<VkAttachmentDescription> attachments(m_Specification.Attachments.Attachments.size());
		uint32_t colorCount = 0;
		bool foundDepth = false;
		for (uint32_t i = 0; i < attachments.size(); i++)
		{
			VkAttachmentDescription attachment{};
			VkFormat format = VulkanUtils::GetVulkanImageFormat(m_Specification.Attachments.Attachments[i].Format);
			attachment.format = format;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			if (VulkanUtils::IsVulkanDepthFormat(format))
			{
				PX_CORE_ASSERT(!foundDepth, "Multiple depthAttachments not allowed!");
				if (!m_DepthAttachment)
					PX_CORE_WARN("VulkanRenderPass::Recreate: Specs.HasDepthAttachment false, but found one!");
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				foundDepth = true;
			}
			else
			{
				if (m_Specification.SwapChainTarget)
					attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				else
					attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorCount++;
			}
			attachments[i] = attachment;
		}

		std::vector<VkAttachmentReference> colorRefs(colorCount);
		VkAttachmentReference depthref{};
		// TODO: if depth attachment is in the middle of color attachments this will break -> ensure depth is always last
		for (uint32_t i = 0; i < attachments.size(); i++)
		{
			if (Utils::IsDepthFormat(m_Specification.Attachments.Attachments[i].Format))
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
	}

	void VulkanFramebuffer::CreateFramebuffer()
	{
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
		if (m_RenderPass)
		{
			vkDestroyRenderPass(device, m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
		}

		CreateAttachments();
		CreateRenderPass();
		CreateFramebuffer();
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
			vkDestroyRenderPass(device, m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
		}
	}

	const Ref<Image2D> VulkanFramebuffer::GetColorAttachment(size_t index)
	{
		PX_CORE_ASSERT(index < m_ColorAttachments.size(), "Index out of range!");
		return m_ColorAttachments[index];
	}
}
