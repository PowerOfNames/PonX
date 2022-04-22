#pragma once
#include "VulkanDevice.h"
#include "VulkanImage2D.h"
#include "VulkanUtility.h"

#include "Povox/Core/Application.h"


#include <vulkan/vulkan.h>
struct GLFWwindow;
namespace Povox {

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
		VulkanSwapchain() = default;
		~VulkanSwapchain() = default;
		void Init(const Ref<VulkanDevice>& device, GLFWwindow* windowHandle);
		void Create(uint32_t width, uint32_t height);
		void Destroy();

		void SwapBuffers();

		void Recreate(uint32_t width, uint32_t height);

		inline VkSwapchainKHR Get() { return m_Swapchain; }
		inline VkSwapchainKHR GetOld() { return m_OldSwapchain; }
		inline VkSurfaceKHR GetSurface() { return m_Surface; }

		inline std::vector<VkImage>& GetImages() { return m_Images; }
		inline std::vector<VkImageView>& GetImageViews() { return m_ImageViews; }
		inline VkRenderPass GetRenderPass() { return m_RenderPass; }
		inline std::vector<VkFramebuffer>& GetFramebuffers() { return m_Framebuffers; }

		inline const VkCommandBuffer GetCurrentCommandBuffer() const { return m_Commandbuffers[m_CurrentFrame].Buffer; }

		// = TotalFrames % MaxFramesInFlight
		inline const uint64_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
		inline const uint64_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }

		inline static const SwapchainProperties& GetProperties() { return m_Props; }
		inline static VkFormat GetImageFormat() { return m_Props.SurfaceFormat.format; }
	private:
		uint32_t AcquireNextImageIndex();
		void QueueSubmit();
		void Present();

		void ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
		void ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		void ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	private:
		static SwapchainProperties m_Props;
		
		Ref<VulkanDevice> m_Device;
		GLFWwindow* m_WindowHandle = nullptr;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkSwapchainKHR m_OldSwapchain = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		uint32_t m_CurrentImageIndex = 0;
		uint64_t m_CurrentFrame = 0;
		uint32_t m_TotalFrames = 0;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		struct CommandBuffer
		{
			VkCommandBuffer Buffer;
			VkCommandPool Pool;
		};
		std::vector<CommandBuffer> m_Commandbuffers;

		struct Semaphores
		{
			VkSemaphore RenderSemaphore;
			VkSemaphore PresentSemaphore;
		};
		std::vector<Semaphores> m_Semaphores;
		std::vector<VkFence> m_Fences;
	};
}
