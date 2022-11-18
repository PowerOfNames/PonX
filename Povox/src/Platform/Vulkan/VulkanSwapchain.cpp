#include "pxpch.h"
#include "VulkanSwapchain.h"

#include "VulkanCommands.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanInitializers.h"

namespace Povox {

	SwapchainProperties VulkanSwapchain::m_Props;

	VulkanSwapchain::VulkanSwapchain(GLFWwindow* windowHandle)
	{
		PX_CORE_ASSERT(windowHandle, "Window handle is null");
		m_WindowHandle = windowHandle;

		PX_CORE_VK_ASSERT(glfwCreateWindowSurface(VulkanContext::GetInstance(), m_WindowHandle, nullptr, &m_Surface), VK_SUCCESS, "Failed to create window surface!");
		PX_CORE_TRACE("VulkanSurface created!");
	}

	void VulkanSwapchain::Init(uint32_t width, uint32_t height)
	{
		Recreate(width, height);
	}

	void VulkanSwapchain::Recreate(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();



		m_Props.Width = width;
		m_Props.Height = height;

		m_OldSwapchain = m_Swapchain;

		VkDevice device = m_Device->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		uint32_t maxFramesInFlight = Application::Get()->GetSpecification().MaxFramesInFlight;

		SwapchainSupportDetails swapchainSupportDetails = m_Device->QuerySwapchainSupport();
		ChooseSwapSurfaceFormat(swapchainSupportDetails.Formats);
		ChooseSwapPresentMode(swapchainSupportDetails.PresentModes);
		ChooseSwapExtent(swapchainSupportDetails.Capabilities, width, height);


		uint32_t imageCount = maxFramesInFlight + 1; // +1 to avoid waiting for driver to finish internal operations
		if (swapchainSupportDetails.Capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.Capabilities.maxImageCount)
			imageCount = swapchainSupportDetails.Capabilities.maxImageCount;
		PX_CORE_TRACE("Choosen SwapchainImageCount: '{0}'", imageCount);

		if (m_Swapchain)
		{
			vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
			m_Swapchain = VK_NULL_HANDLE;
		}

		VkSurfaceCapabilitiesKHR caps{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->GetPhysicalDevice(), m_Surface, &caps);

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_Props.SurfaceFormat.format;
		createInfo.imageColorSpace = m_Props.SurfaceFormat.colorSpace;
		createInfo.imageExtent = { width, width };
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.presentMode = m_Props.PresentMode;
		QueueFamilyIndices queueFamIndices = m_Device->FindQueueFamilies();
		uint32_t queueFamilyIndices[] = {
						queueFamIndices.GraphicsFamily.value(),
						queueFamIndices.PresentFamily.value(),
						queueFamIndices.TransferFamily.value()
		};
		if (queueFamIndices.GraphicsFamily != queueFamIndices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;		// optional
			createInfo.pQueueFamilyIndices = nullptr;	// optional
		}
		createInfo.preTransform = swapchainSupportDetails.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = m_Props.PresentMode;					// -> vsync
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = m_OldSwapchain;
		PX_CORE_VK_ASSERT(vkCreateSwapchainKHR(m_Device->GetVulkanDevice(), &createInfo, nullptr, &m_Swapchain), VK_SUCCESS, "Failed to create logical device!");

		// Images
		vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, m_Images.data());
		m_Props.ImageCount = imageCount;
		m_ImageViews.resize(m_Images.size());
		for (size_t i = 0; i < m_ImageViews.size(); i++)
		{
			VkImageViewCreateInfo imageInfo = VulkanInits::CreateImageViewInfo(m_Props.SurfaceFormat.format, m_Images[i], VK_IMAGE_ASPECT_COLOR_BIT);
			PX_CORE_VK_ASSERT(vkCreateImageView(device, &imageInfo, nullptr, &m_ImageViews[i]), VK_SUCCESS, "Failed to create image view!");
		}

		{
			m_DepthFormat = VulkanUtils::FindDepthFormat(m_Device->GetPhysicalDevice());
			VkExtent3D extent{};
			extent.width = m_Props.Width;
			extent.height = m_Props.Height;
			extent.depth = 1;
			VkImageCreateInfo imageInfo = VulkanInits::CreateImageInfo(m_DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, extent);

			VmaAllocationCreateInfo allocationInfo{};
			allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			PX_CORE_VK_ASSERT(vmaCreateImage(VulkanContext::GetAllocator(), &imageInfo, &allocationInfo, &m_DepthImage, &m_DepthImageAllocation, nullptr), VK_SUCCESS, "Failed to create Image!");

			VkImageViewCreateInfo viewInfo = VulkanInits::CreateImageViewInfo(m_DepthFormat, m_DepthImage, VK_IMAGE_ASPECT_DEPTH_BIT);
			PX_CORE_VK_ASSERT(vkCreateImageView(m_Device->GetVulkanDevice(), &viewInfo, nullptr, &m_DepthImageView), VK_SUCCESS, "Failed to create ImageView!");

			VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS, [=](VkCommandBuffer cmd)
				{
					VkImageMemoryBarrier barrier{};
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.image = m_DepthImage;
					barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (m_DepthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || m_DepthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					barrier.subresourceRange.baseMipLevel = 0;
					barrier.subresourceRange.levelCount = 1;
					barrier.subresourceRange.baseArrayLayer = 0;
					barrier.subresourceRange.layerCount = 1;

					VkPipelineStageFlags srcStage;
					VkPipelineStageFlags dstStage;

					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

					vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
				});
		}

		// RenderPass
		if (!m_RenderPass)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = m_Props.SurfaceFormat.format;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorRef{};
			colorRef.attachment = 0;
			colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = m_DepthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthRef{};
			depthRef.attachment = 1;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorRef;
			subpass.pDepthStencilAttachment = &depthRef;

			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


			std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pNext = nullptr;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassInfo.pAttachments = attachments.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			PX_CORE_VK_ASSERT(vkCreateRenderPass(m_Device->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass), VK_SUCCESS, "Failed to create renderpass!");
		}

		// Framebuffers
		m_Framebuffers.resize(m_ImageViews.size());
		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.pNext = nullptr;
		fbInfo.width = width;
		fbInfo.height = height;
		fbInfo.renderPass = m_RenderPass;
		fbInfo.layers = 1;
		for (size_t i = 0; i < m_ImageViews.size(); i++)
		{
			std::array<VkImageView, 2> imageViews = { m_ImageViews[i], m_DepthImageView };
			fbInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
			fbInfo.pAttachments = imageViews.data();
			PX_CORE_VK_ASSERT(vkCreateFramebuffer(m_Device->GetVulkanDevice(), &fbInfo, nullptr, &m_Framebuffers[i]), VK_SUCCESS, "Failed to create framebuffer!");
		}

		PX_CORE_INFO("Finished Swapchain creation with extent: '( {0} | {1} )', '{2}' Images and '{3}' Frames in Flight!", width, height, m_Images.size(), maxFramesInFlight);
	}

	void VulkanSwapchain::Destroy()
	{
		PX_PROFILE_FUNCTION();


		VkDevice device = m_Device->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		for (size_t i = 0; i < m_Framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(device, m_Framebuffers[i], nullptr);
		}

		if (m_RenderPass)
			vkDestroyRenderPass(device, m_RenderPass, nullptr);

		for (size_t i = 0; i < m_Images.size(); i++)
		{
			vkDestroyImage(device, m_Images[i], nullptr);
			vkDestroyImageView(device, m_ImageViews[i], nullptr);
		}
		vmaDestroyImage(VulkanContext::GetAllocator(), m_DepthImage, m_DepthImageAllocation);
		vkDestroyImageView(device, m_DepthImageView, nullptr);

		if (m_Swapchain)
			vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
		if(m_OldSwapchain)
			vkDestroySwapchainKHR(device, m_OldSwapchain, nullptr);

		vkDestroySurfaceKHR(VulkanContext::GetInstance(), m_Surface, nullptr);
	}

	SwapchainFrame* VulkanSwapchain::AcquireNextImageIndex(VkSemaphore presentSemaphore)
	{
		PX_PROFILE_FUNCTION();


		m_CurrentFrame.LastImageIndex = m_CurrentFrame.CurrentImageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device->GetVulkanDevice(), m_Swapchain, UINT64_MAX /*disables timeout*/, presentSemaphore, VK_NULL_HANDLE, &m_CurrentFrame.CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Recreate(Application::Get()->GetWindow().GetWidth(), Application::Get()->GetWindow().GetHeight()); // get the new size maybe from elsewhere -> might be that the size change caused the result!
			//TODO: investigate if this works!
			return AcquireNextImageIndex(presentSemaphore);
		}
		else
		{
			PX_CORE_ASSERT(!((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)), "Failed to acquire swap chain image!");
		}

		m_CurrentFrame.CurrentImage = m_Images[m_CurrentFrame.CurrentImageIndex];
		m_CurrentFrame.CurrentImageView = m_ImageViews[m_CurrentFrame.CurrentImageIndex];

		return &m_CurrentFrame;
	}

	void VulkanSwapchain::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();

		//Commandstuff NOW

		QueueSubmit();
		Present();
		PX_CORE_WARN("Finished swap buffers!");
	}	

	void VulkanSwapchain::QueueSubmit()
	{
		PX_PROFILE_FUNCTION();


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.pWaitSemaphores = &m_CurrentFrame.PresentSemaphore;
		submitInfo.commandBufferCount = static_cast<uint32_t>(m_CurrentFrame.Commands.size());
		submitInfo.pCommandBuffers = m_CurrentFrame.Commands.data();

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_CurrentFrame.RenderSemaphore;

		PX_CORE_VK_ASSERT(vkQueueSubmit(m_Device->GetQueueFamilies().GraphicsQueue, 1, &submitInfo, m_CurrentFrame.CurrentFence), VK_SUCCESS, "Failed to submit draw render buffer!");
	}

	void VulkanSwapchain::Present()
	{
		PX_PROFILE_FUNCTION();


		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pWaitSemaphores = &m_CurrentFrame.RenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pImageIndices = &m_CurrentFrame.CurrentImageIndex;
		presentInfo.pResults = nullptr;

		VkResult result;
		result = vkQueuePresentKHR(m_Device->GetQueueFamilies().PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			Recreate(Application::Get()->GetWindow().GetWidth(), Application::Get()->GetWindow().GetHeight());
		}
		else
		{
			PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");
		}
	}


	void VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		VkSurfaceFormatKHR result;
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				result = format;
				PX_CORE_INFO("Swapchain surface format:  {0}", result.format);
				m_Props.SurfaceFormat = format;
			}
		}
		result = availableFormats[0];
		PX_CORE_INFO("Swapchain surface format:  {0}", result.format);
		m_Props.SurfaceFormat = result;
	}
	void VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& mode : availablePresentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // on mobile, FIFO_KHR is more likely to be used because of lower enegry consumption
				m_Props.PresentMode = mode;
		}
		m_Props.PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	}
	void VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			m_Props.Width = capabilities.currentExtent.width;
			m_Props.Height = capabilities.currentExtent.height;
		}

		else
		{
			VkExtent2D actualExtend = { width, height };

			actualExtend.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtend.width));
			actualExtend.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtend.height));

			m_Props.Width = actualExtend.width;
			m_Props.Height = actualExtend.height;
		}
		PX_CORE_INFO("Swapchain extent: '[{0}|{1}]'", m_Props.Width, m_Props.Height);
	}
}
