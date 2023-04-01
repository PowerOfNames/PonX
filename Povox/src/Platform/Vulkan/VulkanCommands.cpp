#include "pxpch.h"
#include "VulkanCommands.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanUtilities.h"

namespace Povox {

// ========== Commands ==========
	CommandControlState VulkanCommandControl::s_CommandControlState{};
	Ref<UploadContext> VulkanCommandControl::s_UploadContext = nullptr;

	void VulkanCommandControl::ImmidiateSubmit(SubmitType submitType, std::function<void(VkCommandBuffer cmd)>&& function)
	{
		if (!s_CommandControlState.IsInitialized)
		{
			PX_CORE_ERROR("UploadContext has to be set first!");
			return;
		}

		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandBuffer buffer = VK_NULL_HANDLE;
		VkQueue queue = VK_NULL_HANDLE;
		switch(submitType)
		{
			case SubmitType::SUBMIT_TYPE_GRAPHICS:
			{
				pool = s_UploadContext->CmdPoolGfx;
				buffer = s_UploadContext->CmdBufferGfx;
				queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				break;
			}
			case SubmitType::SUBMIT_TYPE_TRANSFER:
			{
				pool = s_UploadContext->CmdPoolTrsf;
				buffer = s_UploadContext->CmdBufferTrsf;
				queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.TransferQueue;
				break;
			}
		}
		if (!pool || !buffer || !queue)
		{
			PX_CORE_ASSERT(true, "Upload command type undefined!");
			return;
		}
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();


		VkCommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBeginInfo.pInheritanceInfo = nullptr;

		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(buffer, &cmdBeginInfo), VK_SUCCESS, "Failed to begin immidite submit cmd buffer!");
		function(buffer);
		PX_CORE_VK_ASSERT(vkEndCommandBuffer(buffer), VK_SUCCESS, "Failed to end cmd buffer!");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;

		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pCommandBuffers = &buffer;
		submitInfo.pSignalSemaphores = nullptr;
		PX_CORE_VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, s_UploadContext->Fence), VK_SUCCESS, "Failed to submit cmd buffer!");
		
		vkWaitForFences(device, 1, &s_UploadContext->Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &s_UploadContext->Fence);

		PX_CORE_VK_ASSERT(vkResetCommandPool(device, pool, 0), VK_SUCCESS, "Failed to reset command pool!");
	}

	Ref<UploadContext> VulkanCommandControl::CreateUploadContext()
	{
		PX_CORE_INFO("VulkanCommandControl::CreateUploadContext: Creating...");

		//TODO: Overhaul and align with different Queue setups!
		
		s_UploadContext = CreateRef<UploadContext>();
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		auto& families = VulkanContext::GetDevice()->GetQueueFamilies();

		VkCommandPoolCreateInfo uploadPoolInfoTrsf = {};
		uploadPoolInfoTrsf.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		uploadPoolInfoTrsf.pNext = nullptr;

		uploadPoolInfoTrsf.queueFamilyIndex = families.TransferFamilyIndex;
		uploadPoolInfoTrsf.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &uploadPoolInfoTrsf, nullptr, &s_UploadContext->CmdPoolTrsf), VK_SUCCESS, "Failed to create graphics command pool!");


		VkCommandBufferAllocateInfo uploadCmdBufferTrsfInfo{};
		uploadCmdBufferTrsfInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		uploadCmdBufferTrsfInfo.pNext = nullptr;

		uploadCmdBufferTrsfInfo.commandPool = s_UploadContext->CmdPoolTrsf;
		uploadCmdBufferTrsfInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		uploadCmdBufferTrsfInfo.commandBufferCount = 1;
		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &uploadCmdBufferTrsfInfo, &s_UploadContext->CmdBufferTrsf), VK_SUCCESS, "Failed to create CommandBuffer!");



		VkCommandPoolCreateInfo uploadPoolInfoGfx = {};
		uploadPoolInfoGfx.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		uploadPoolInfoGfx.pNext = nullptr;

		uploadPoolInfoGfx.queueFamilyIndex = families.GraphicsFamilyIndex;
		uploadPoolInfoGfx.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &uploadPoolInfoGfx, nullptr, &s_UploadContext->CmdPoolGfx), VK_SUCCESS, "Failed to create graphics command pool!");

		VkCommandBufferAllocateInfo uploadCmdBufferGfxInfo{};
		uploadCmdBufferGfxInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		uploadCmdBufferGfxInfo.pNext = nullptr;

		uploadCmdBufferGfxInfo.commandPool = s_UploadContext->CmdPoolGfx;
		uploadCmdBufferGfxInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		uploadCmdBufferGfxInfo.commandBufferCount = 1;
		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &uploadCmdBufferGfxInfo, &s_UploadContext->CmdBufferGfx), VK_SUCCESS, "Failed to create CommandBuffer!");

		s_CommandControlState.IsInitialized = true;

		PX_CORE_INFO("VulkanCommandControl::CreateUploadContext: Completed.");
		return s_UploadContext;
	}

}
