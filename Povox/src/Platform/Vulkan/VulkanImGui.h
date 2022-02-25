#pragma once
#include "VulkanUtility.h"

#include "VulkanBuffer.h"

struct GLFWwindow;

namespace Povox {


	class VulkanImGui
	{
	public:
		struct FrameData
		{
			VkCommandPool CommandPool{ VK_NULL_HANDLE };
			VkCommandBuffer CommandBuffer{ VK_NULL_HANDLE };

			VkDescriptorSet DescriptorSet{ VK_NULL_HANDLE };
		};

		VulkanImGui(VulkanCoreObjects* core, UploadContext* uploadContext, VkFormat swapchainImageFormat, uint8_t maxFrames);
		~VulkanImGui() = default;

		void Init(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height);

		void OnSwapchainRecreate(const std::vector<VkImageView>& swapchainImageViews, VkExtent2D newExtent);

		void Destroy();
		void DestroyRenderPass();
		void VulkanImGui::DestroyFramebuffers();
		void VulkanImGui::DestroyCommands();

		void BeginFrame();
		void EndFrame();

		VkCommandBuffer BeginRender(uint32_t imageIndex, VkExtent2D swapchainExtent);
		void RenderDrawData(VkCommandBuffer cmd);
		void EndRender(VkCommandBuffer cmd);

		FrameData& GetFrame(uint32_t index);
		inline VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }

	private:
		void InitRenderPass();
		void InitCommandBuffers();
		void InitFrameBuffers(const std::vector<VkImageView>& swapchainViews, uint32_t width, uint32_t height);
	
	private:
		VulkanCoreObjects* m_Core;
		UploadContext* m_UploadContext;
		VkFormat m_SwapchainImageFormat;

		VkDescriptorPool m_DescriptorPool;

		uint8_t m_MaxFramesInFlight;
		std::vector<VulkanImGui::FrameData> m_FrameData = {};
		std::vector<VkFramebuffer> m_Framebuffers = {};

		VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
	};


}
