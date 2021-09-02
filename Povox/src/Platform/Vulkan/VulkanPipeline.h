#pragma once
#include <vulkan/vulkan.h>

#include "VulkanSwapchain.h"
#include "VulkanShader.h"

namespace Povox {

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass() = default;
		~VulkanRenderPass() = default;

		void Create(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkFormat swapchainImageFormat);
		void Destroy(VkDevice logicalDevice);

		VkRenderPass Get() const { return m_RenderPass; }

	private:
		VkRenderPass m_RenderPass;
	};

	class VulkanPipeline
	{
	public:
		VulkanPipeline() = default;
		~VulkanPipeline() = default;


		void Create(VkDevice logicalDevice, VkExtent2D swapchainExtent, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
		void Destroy(VkDevice logicalDevice);
		void DestroyLayout(VkDevice logicalDevice);

		VkPipelineLayout GetLayout() const { return m_Layout; }
		VkPipeline Get() const { return m_Pipeline; }

	private:
		VkPipelineLayout m_Layout;
		VkPipeline m_Pipeline;
	};

}