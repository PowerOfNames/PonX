#include "pxpch.h"
#include "VulkanCommands.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanUtilities.h"
#include "VulkanInitializers.h"

namespace Povox {

// ========== Commands ==========
	Ref<UploadContext> VulkanCommandControl::m_UploadContext = nullptr;

	void VulkanCommandControl::ImmidiateSubmit(SubmitType submitType, std::function<void(VkCommandBuffer cmd)>&& function)
	{
		if (!m_UploadContext)
		{
			PX_CORE_ASSERT(true, "UploadCOntext has to be set first!");
			return;
		}

		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandBuffer buffer = VK_NULL_HANDLE;
		VkQueue queue = VK_NULL_HANDLE;
		switch(submitType)
		{
		case SubmitType::SUBMIT_TYPE_GRAPHICS:
		{
			pool = m_UploadContext->CmdPoolGfx;
			buffer = m_UploadContext->CmdBufferGfx;
			queue = VulkanContext::GetDevice()->GetQueueFamilies().GraphicsQueue;
			break;
		}
		case SubmitType::SUBMIT_TYPE_TRANSFER:
		{
			pool = m_UploadContext->CmdPoolTrsf;
			buffer = m_UploadContext->CmdBufferTrsf;
			VulkanContext::GetDevice()->GetQueueFamilies().TransferQueue;
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
		PX_CORE_VK_ASSERT(vkQueueSubmit(VulkanContext::GetDevice()->GetQueueFamilies().GraphicsQueue, 1, &submitInfo, m_UploadContext->Fence), VK_SUCCESS, "Failed to submit cmd buffer!");
		
		vkWaitForFences(device, 1, &m_UploadContext->Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &m_UploadContext->Fence);

		PX_CORE_VK_ASSERT(vkResetCommandPool(device, pool, 0), VK_SUCCESS, "Failed to reset command pool!");
	}

	Ref<UploadContext> VulkanCommandControl::CreateUploadContext()
	{
		m_UploadContext = CreateRef<UploadContext>();
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		auto& families = VulkanContext::GetDevice()->FindQueueFamilies();

		VkCommandPoolCreateInfo uploadPoolInfoTrsf = {};
		uploadPoolInfoTrsf.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		uploadPoolInfoTrsf.pNext = nullptr;

		uploadPoolInfoTrsf.queueFamilyIndex = families.TransferFamily.value();
		uploadPoolInfoTrsf.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &uploadPoolInfoTrsf, nullptr, &m_UploadContext->CmdPoolTrsf), VK_SUCCESS, "Failed to create graphics command pool!");


		VkCommandBufferAllocateInfo uploadCmdBufferTrsfInfo{};
		uploadCmdBufferTrsfInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		uploadCmdBufferTrsfInfo.pNext = nullptr;

		uploadCmdBufferTrsfInfo.commandPool = m_UploadContext->CmdPoolTrsf;
		uploadCmdBufferTrsfInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		uploadCmdBufferTrsfInfo.commandBufferCount = 1;
		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &uploadCmdBufferTrsfInfo, &m_UploadContext->CmdBufferTrsf), VK_SUCCESS, "Failed to create CommandBuffer!");



		VkCommandPoolCreateInfo uploadPoolInfoGfx = {};
		uploadPoolInfoGfx.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		uploadPoolInfoGfx.pNext = nullptr;

		uploadPoolInfoGfx.queueFamilyIndex = families.GraphicsFamily.value();
		uploadPoolInfoGfx.flags = 0;
		PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &uploadPoolInfoGfx, nullptr, &m_UploadContext->CmdPoolGfx), VK_SUCCESS, "Failed to create graphics command pool!");

		VkCommandBufferAllocateInfo uploadCmdBufferGfxInfo{};
		uploadCmdBufferGfxInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		uploadCmdBufferGfxInfo.pNext = nullptr;

		uploadCmdBufferGfxInfo.commandPool = m_UploadContext->CmdPoolTrsf;
		uploadCmdBufferGfxInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// primary can be called for execution, secondary can be called from primaries		
		uploadCmdBufferGfxInfo.commandBufferCount = 1;
		PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &uploadCmdBufferGfxInfo, &m_UploadContext->CmdBufferGfx), VK_SUCCESS, "Failed to create CommandBuffer!");

		return m_UploadContext;
	}

}
