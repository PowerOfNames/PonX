#include "pxpch.h"
#include "VulkanSwapchain.h"

#include "VulkanDebug.h"

namespace Povox {

	void VulkanSwapchain::Destroy(VkDevice logicalDevice)
	{
		vkDestroySwapchainKHR(logicalDevice, m_Swapchain, nullptr);
	}

	void VulkanSwapchain::Create(VkDevice logicalDevice, VkSurfaceKHR surface, SwapchainSupportDetails swapchainSupportDetails, QueueFamilyIndices indices, int width, int height)
	{
		ChooseSwapSurfaceFormat(swapchainSupportDetails.Formats);
		ChooseSwapPresentMode(swapchainSupportDetails.PresentModes);
		ChooseSwapExtent(swapchainSupportDetails.Capabilities, width, height);

		uint32_t imageCount = swapchainSupportDetails.Capabilities.minImageCount + 1; // +1 to avoid waiting for driver to finish internal operations
		if (swapchainSupportDetails.Capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.Capabilities.maxImageCount)
			imageCount = swapchainSupportDetails.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_SurfaceFormat.format;
		createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
		createInfo.imageExtent = m_Extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.presentMode = m_PresentMode;

		m_ImageFormat = m_SurfaceFormat.format;
		m_ImageCount = imageCount;


		uint32_t queueFamilyIndices[] = {
						indices.GraphicsFamily.value(),
						indices.PresentFamily.value(),
						indices.TransferFamily.value()
		};

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;		// optional
			createInfo.pQueueFamilyIndices = nullptr;	// optianal
		}

		createInfo.preTransform = swapchainSupportDetails.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = m_PresentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;


		PX_CORE_VK_ASSERT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &m_Swapchain), VK_SUCCESS, "Failed to create logical device!");
	}

	void VulkanSwapchain::CreateImagesAndViews(VkDevice logicalDevice)
	{
		vkGetSwapchainImagesKHR(logicalDevice, m_Swapchain, &m_ImageCount, nullptr);
		m_Images.resize(m_ImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, m_Swapchain, &m_ImageCount, m_Images.data());

		m_ImageViews.resize(m_Images.size());

		for (size_t i = 0; i < m_ImageViews.size(); i++)
		{
			 VulkanImageView::Create(logicalDevice, m_ImageViews[i], m_Images[i], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				m_SurfaceFormat = format;
		}
		m_SurfaceFormat = availableFormats[0];
	}


	void VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& mode : availablePresentModes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) // on mobile, FIFO_KHR is more likely to be used because of lower enegry consumption
				m_PresentMode = mode;
		}
		m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	}


	void VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			m_Extent = capabilities.currentExtent;

		else
		{
			VkExtent2D actualExtend = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtend.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtend.width));
			actualExtend.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtend.height));

			m_Extent = actualExtend;
		}
	}
}