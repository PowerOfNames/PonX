#include "pxpch.h"
#include "VulkanCommands.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanUtility.h"
#include "VulkanInitializers.h"

namespace Povox {

// ========== Commands ==========
	void VulkanCommands::ImmidiateSubmitGfx(UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VkCommandBuffer cmd = uploadContext.CmdBufferGfx;
		VkCommandBufferBeginInfo cmdBeginInfo = VulkanInits::BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(cmd, &cmdBeginInfo), VK_SUCCESS, "Failed to begin immidite submit cmd buffer!");

		function(cmd);

		PX_CORE_VK_ASSERT(vkEndCommandBuffer(cmd), VK_SUCCESS, "Failed to end cmd buffer!");
		VkSubmitInfo submitInfo = VulkanInits::SubmitInfo(&cmd);

		PX_CORE_VK_ASSERT(vkQueueSubmit(VulkanContext::GetDevice()->GetQueueFamilies().GraphicsQueue, 1, &submitInfo, uploadContext.Fence), VK_SUCCESS, "Failed to submit cmd buffer!");
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkWaitForFences(device, 1, &uploadContext.Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &uploadContext.Fence);

		PX_CORE_VK_ASSERT(vkResetCommandPool(device, uploadContext.CmdPoolGfx, 0), VK_SUCCESS, "Failed to reset command pool!");
	}

	void VulkanCommands::ImmidiateSubmitTrsf(UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VkCommandBuffer cmd = uploadContext.CmdBufferTrsf;
		VkCommandBufferBeginInfo cmdBeginInfo = VulkanInits::BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		vkBeginCommandBuffer(cmd, &cmdBeginInfo);

		function(cmd);

		PX_CORE_VK_ASSERT(vkEndCommandBuffer(cmd), VK_SUCCESS, "Failed to end cmd buffer!");
		VkSubmitInfo submitInfo = VulkanInits::SubmitInfo(&cmd);

		PX_CORE_VK_ASSERT(vkQueueSubmit(VulkanContext::GetDevice()->GetQueueFamilies().TransferQueue, 1, &submitInfo, uploadContext.Fence), VK_SUCCESS, "Failed to submit cmd buffer!");
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkWaitForFences(device, 1, &uploadContext.Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &uploadContext.Fence);

		PX_CORE_VK_ASSERT(vkResetCommandPool(device, uploadContext.CmdPoolTrsf, 0), VK_SUCCESS, "Failed to reset command pool!");
	}
	


// ========== CommanPool ==========
	VkCommandPool VulkanCommandPool::Create(VkCommandPoolCreateInfo commandPoolInfo)
	{
		VkCommandPool pool;
		PX_CORE_VK_ASSERT(vkCreateCommandPool(VulkanContext::GetDevice()->GetVulkanDevice(), &commandPoolInfo, nullptr, &pool), VK_SUCCESS, "Failed to create graphics command pool!");
		return pool;
	}

	void VulkanCommandPool::Destroy(VkDevice device, VkCommandPool& pool)
	{
		vkDestroyCommandPool(device, pool, nullptr);
	}


// ========== CommandBuffer ==========
	VkCommandBuffer VulkanCommandBuffer::Create(VkDevice device, VkCommandPool& commandPool, VkCommandBufferAllocateInfo bufferInfo)
	{
		VkCommandBuffer buffer;
		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &bufferInfo, &buffer), VK_SUCCESS, "Failed to create CommandBuffer!");
		return buffer;
	}
}
