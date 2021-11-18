#pragma once

#include <vulkan/vulkan.h>

#include "VulkanImage.h"
#include "VulkanUtility.h"

struct GLFWwindow;
namespace Povox {

	class VulkanSwapchain
	{
	public:
		VulkanSwapchain() = default;
		~VulkanSwapchain() = default;

		void Destroy(VkDevice logicalDevice);

		void Create(VulkanCoreObjects& core, SwapchainSupportDetails swapchainSupportDetails, int width, int height);
		void CreateImagesAndViews(VkDevice logicalDevice);

		VkSwapchainKHR Get() const { return m_Swapchain; }
		VkSwapchainKHR Get() { return m_Swapchain; }
		std::vector<VkImage> GetImages() { return m_Images; }
		std::vector<VkImageView> GetImageViews() { return m_ImageViews; }

		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent2D() const { return m_Extent; }

	private:
		void ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);
		void ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		void ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	private:
		VkSwapchainKHR m_Swapchain;
		VkFormat m_ImageFormat;

		VkExtent2D m_Extent;
		VkSurfaceFormatKHR m_SurfaceFormat;
		VkPresentModeKHR m_PresentMode;

		uint32_t m_ImageCount;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
	};
}
