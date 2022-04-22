#include "pxpch.h"
#include "VulkanSwapchain.h"

#include "VulkanContext.h"

#include "VulkanInitializers.h"
#include "VulkanDebug.h"

namespace Povox {

	SwapchainProperties VulkanSwapchain::m_Props;

	void VulkanSwapchain::Init(const Ref<VulkanDevice>& device, GLFWwindow* windowHandle)
	{
		m_Device = device;
		m_WindowHandle = windowHandle;

		PX_CORE_ASSERT(windowHandle, "Window handle is null");

		PX_CORE_VK_ASSERT(glfwCreateWindowSurface(VulkanContext::GetInstance(), m_WindowHandle, nullptr, &m_Surface), VK_SUCCESS, "Failed to create window surface!");
		PX_CORE_TRACE("VulkanSurface created!");
	}

	void VulkanSwapchain::Create(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		m_Props.Width = width;
		m_Props.Height = height;

		m_OldSwapchain = m_Swapchain;

		VkDevice device = m_Device->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		uint32_t maxFramesInFlight = Application::GetSpecification().RendererProps.MaxFramesInFlight;

		SwapchainSupportDetails swapchainSupportDetails = m_Device->QuerySwapchainSupport();
		ChooseSwapSurfaceFormat(swapchainSupportDetails.Formats);
		ChooseSwapPresentMode(swapchainSupportDetails.PresentModes);
		ChooseSwapExtent(swapchainSupportDetails.Capabilities, width, height);


		uint32_t imageCount = maxFramesInFlight + 1; // +1 to avoid waiting for driver to finish internal operations
		if (swapchainSupportDetails.Capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.Capabilities.maxImageCount)
			imageCount = swapchainSupportDetails.Capabilities.maxImageCount;
		PX_CORE_TRACE("Choosen SwapchainImageCount: '{0}'", imageCount);

		if (m_Swapchain)
			vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
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

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorRef;

			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pNext = nullptr;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
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
			fbInfo.attachmentCount = 1;
			fbInfo.pAttachments = &m_ImageViews[i];
			PX_CORE_VK_ASSERT(vkCreateFramebuffer(m_Device->GetVulkanDevice(), &fbInfo, nullptr, &m_Framebuffers[i]), VK_SUCCESS, "Failed to create framebuffer!");
		}		

		// Semaphores
		if (m_Semaphores.size() != maxFramesInFlight)
		{
			m_Semaphores.resize(maxFramesInFlight);

			VkSemaphoreCreateInfo createSemaphoreInfo{};
			createSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
						

			for (size_t i = 0; i < maxFramesInFlight; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateSemaphore(device, &createSemaphoreInfo, nullptr, &m_Semaphores[i].RenderSemaphore), VK_SUCCESS, "Failed to create RenderSemaphore!");
				PX_CORE_VK_ASSERT(vkCreateSemaphore(device, &createSemaphoreInfo, nullptr, &m_Semaphores[i].PresentSemaphore), VK_SUCCESS, "Failed to create PresentSemaphore!");
			}
		}

		// Fences
		if (m_Fences.size() != maxFramesInFlight)
		{
			m_Fences.resize(maxFramesInFlight);

			VkFenceCreateInfo createFenceInfo{};
			createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for (size_t i = 0; i < maxFramesInFlight; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateFence(device, &createFenceInfo, nullptr, &m_Fences[i]), VK_SUCCESS, "Failed to create Fence!");
			}
		}

		//CommandBuffers
		if (m_Commandbuffers.size() != maxFramesInFlight)
		{
			m_Commandbuffers.resize(maxFramesInFlight);

			VkCommandPoolCreateInfo poolci{};
			poolci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolci.pNext = nullptr;
			poolci.queueFamilyIndex = m_Device->FindQueueFamilies().GraphicsFamily.value();
			poolci.flags = 0;

			VkCommandBufferAllocateInfo bufferci{};
			bufferci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			bufferci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			bufferci.commandBufferCount = 1;

			for (int i = 0; i < maxFramesInFlight; i++)
			{
				PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &poolci, nullptr, &m_Commandbuffers[i].Pool), VK_SUCCESS, "Failed to create graphics command pool!");

				bufferci.commandPool = m_Commandbuffers[i].Pool;
				PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &bufferci, &m_Commandbuffers[i].Buffer), VK_SUCCESS, "Failed to create CommandBuffer!");
			}
		}

		PX_CORE_INFO("Finished Swapchain creation with extent: '( {0} | {1} )', '{2}' Images and '{3}' Frames in Flight!", width, height, m_Images.size(), maxFramesInFlight);
	}

	void VulkanSwapchain::Destroy()
	{
		PX_PROFILE_FUNCTION();


		VkDevice device = m_Device->GetVulkanDevice();
		vkDeviceWaitIdle(device);
		
		for (size_t i = 0; i < m_Semaphores.size(); i++)
		{
			vkDestroySemaphore(device, m_Semaphores[i].PresentSemaphore, nullptr);
			vkDestroySemaphore(device, m_Semaphores[i].RenderSemaphore, nullptr);
		}

		for (size_t i = 0; i < m_Fences.size(); i++)
		{
			vkDestroyFence(device, m_Fences[i], nullptr);
		}

		for (size_t i = 0; i < m_Commandbuffers.size(); i++)
		{
			vkDestroyCommandPool(device, m_Commandbuffers[i].Pool, nullptr);
		}

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

		if (m_Swapchain)
			vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
		if(m_OldSwapchain)
			vkDestroySwapchainKHR(device, m_OldSwapchain, nullptr);

		vkDestroySurfaceKHR(VulkanContext::GetInstance(), m_Surface, nullptr);
	}

	void VulkanSwapchain::SwapBuffers()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_WARN("Start swap buffers!");
		vkWaitForFences(m_Device->GetVulkanDevice(), 1, &m_Fences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device->GetVulkanDevice(), 1, &m_Fences[m_CurrentFrame]);

		vkResetCommandPool(m_Device->GetVulkanDevice(), m_Commandbuffers[m_CurrentFrame].Pool, 0);

		m_CurrentImageIndex = AcquireNextImageIndex();

		//now the command buffer recording should happen!
		// Renderer::GetCommandQueue().execute() ... or something like that!

		QueueSubmit();
		Present();
		PX_CORE_WARN("Finished swap buffers!");
	}

	void VulkanSwapchain::Recreate(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		Create(width, height);
	}

	uint32_t VulkanSwapchain::AcquireNextImageIndex()
	{
		PX_PROFILE_FUNCTION();


		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Device->GetVulkanDevice(), m_Swapchain, UINT64_MAX /*disables timeout*/, m_Semaphores[m_CurrentFrame].PresentSemaphore, VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Create(Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight()); // get the new size maybe from elsewhere -> might be that the size change caused the result!
			return;
		}
		else
		{
			PX_CORE_ASSERT(!((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)), "Failed to acquire swap chain image!");
		}

		return imageIndex;
	}

	void VulkanSwapchain::Present()
	{
		PX_PROFILE_FUNCTION();


		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pWaitSemaphores = &m_Semaphores[m_CurrentFrame].RenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pImageIndices = &m_CurrentImageIndex;
		presentInfo.pResults = nullptr;

		VkResult result;
		result = vkQueuePresentKHR(m_Device->GetQueueFamilies().PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			Create(Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight());
		}
		else
		{
			PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to present swap chain image!");
		}

		m_TotalFrames++;
		m_CurrentFrame = m_CurrentFrame++ % Application::GetSpecification().RendererProps.MaxFramesInFlight;
		PX_CORE_WARN("Current Frame: '{0}'", m_CurrentFrame);
	}

	void VulkanSwapchain::QueueSubmit()
	{
		PX_PROFILE_FUNCTION();


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.pWaitSemaphores = &m_Semaphores[m_CurrentFrame].PresentSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_Commandbuffers[m_CurrentFrame].Buffer;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_Semaphores[m_CurrentFrame].RenderSemaphore;

		PX_CORE_VK_ASSERT(vkQueueSubmit(m_Device->GetQueueFamilies().GraphicsQueue, 1, &submitInfo, m_Fences[m_CurrentFrame]), VK_SUCCESS, "Failed to submit draw render buffer!");
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
