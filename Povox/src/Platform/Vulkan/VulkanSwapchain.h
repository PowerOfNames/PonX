#pragma once
#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanImage2D.h"



struct GLFWwindow;
namespace Povox {

	struct SwapchainFrame
	{
		uint32_t LastImageIndex = 0; //The image that got rendered last
		uint32_t CurrentImageIndex = 0; //The index of the image that gets rendered next

		VkImage CurrentImage = VK_NULL_HANDLE;
		VkImageView CurrentImageView = VK_NULL_HANDLE;

		VkFence CurrentFence = VK_NULL_HANDLE;
		VkSemaphore PresentSemaphore = VK_NULL_HANDLE;
		VkSemaphore RenderSemaphore = VK_NULL_HANDLE;

		std::vector<VkCommandBuffer> Commands;
	};

	struct SwapchainProperties
	{
		uint32_t Width = 0, Height = 0;
		uint32_t ImageCount = 0;

		VkSurfaceFormatKHR SurfaceFormat{};
		VkPresentModeKHR PresentMode{};
	};


	class VulkanSwapchain
	{
	public:
		VulkanSwapchain(GLFWwindow* m_WindowHandle);
		~VulkanSwapchain() = default;
		void Init(uint32_t width, uint32_t height);
		void Destroy();

		void SwapBuffers();
		

		void Recreate(uint32_t width, uint32_t height);

		inline VkSwapchainKHR Get() { return m_Swapchain; }
		//inline VkSwapchainKHR GetOld() { return m_OldSwapchain; }

		inline std::vector<VkImage>& GetImages() { return m_Images; }
		inline std::vector<VkImageView>& GetImageViews() { return m_ImageViews; }
		inline static VkFormat GetImageFormat() { return s_Props.SurfaceFormat.format; }

		inline VkRenderPass GetRenderPass() { return m_RenderPass; }
		inline std::vector<VkFramebuffer>& GetFramebuffers() { return m_Framebuffers; }

		inline static VkSurfaceKHR GetSurface() { return s_Surface; }
		inline static const SwapchainProperties& GetProperties() { return s_Props; }

		SwapchainFrame* AcquireNextImageIndex(VkSemaphore presentSemaphore);

	private:
		void QueueSubmit();
		void Present();

		void Cleanup();
		void ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
		void ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		void ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	private:
		static SwapchainProperties s_Props;
		static VkSurfaceKHR s_Surface;
		
		GLFWwindow* m_WindowHandle = nullptr;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkSwapchainKHR m_OldSwapchain = VK_NULL_HANDLE;


		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		VkImage m_DepthImage = VK_NULL_HANDLE;
		VmaAllocation m_DepthImageAllocation = nullptr;
		VkImageView m_DepthImageView = VK_NULL_HANDLE;
		VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;

		std::vector<VkFramebuffer> m_Framebuffers;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		SwapchainFrame m_CurrentFrame;
	};
}
