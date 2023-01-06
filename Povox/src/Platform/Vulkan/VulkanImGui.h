#pragma once
#include "VulkanUtilities.h"

#include "VulkanBuffer.h"

struct GLFWwindow;

namespace Povox {


	class VulkanImGui
	{
	public:
		struct FrameData
		{
			VkCommandPool CommandPool = VK_NULL_HANDLE;
			VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;

			VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
		};

		VulkanImGui(VkFormat swapchainImageFormat, uint8_t maxFrames);
		~VulkanImGui();

		void Init(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height);

		void OnSwapchainRecreate(const std::vector<VkImageView>& swapchainImageViews, VkExtent2D newExtent);

		void Destroy();

		void BeginFrame();
		void EndFrame();

		void BeginRender(uint32_t imageIndex, VkExtent2D swapchainExtent);
		void RenderDrawData();
		void EndRender();

		FrameData& GetFrame(uint32_t index);
		inline VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }

	private:
		void InitRenderPass();
		void InitCommandBuffers();
		void InitFrameBuffers(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height);
	
	private:
		VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

		uint8_t m_MaxFramesInFlight = 0;
		std::vector<VulkanImGui::FrameData> m_FrameData ;
		std::vector<VkFramebuffer> m_Framebuffers ;

		VkCommandBuffer m_CurrentActiveCmd = VK_NULL_HANDLE;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};


}
