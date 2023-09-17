#include "pxpch.h"
#include "VulkanCommands.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanUtilities.h"

namespace Povox {

// ========== Commands ==========
	CommandControlState VulkanCommandControl::s_CommandControlState{};
	Ref<UploadContext> VulkanCommandControl::s_UploadContext = nullptr;
		

	void VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(SubmitType submitType,
		std::function<void(VkCommandBuffer releasingCmd)>&& releaseFunction,
		std::function<void(VkCommandBuffer acquireCmd) >&& acquireFunction)
	{
		if (!s_CommandControlState.IsInitialized)
		{
			PX_CORE_ERROR("UploadContext has to be set first!");
			return;
		}

		VkCommandPool currentPool = VK_NULL_HANDLE;
		VkCommandBuffer currentBuffer = VK_NULL_HANDLE;
		VkQueue currentQueue = VK_NULL_HANDLE;
		VkFence currentFence = VK_NULL_HANDLE;
		switch (submitType)
		{
			case SubmitType::SUBMIT_TYPE_GRAPHICS_COMPUTE:
			case SubmitType::SUBMIT_TYPE_GRAPHICS_TRANSFER:
			{
				currentPool = s_UploadContext->GraphicsCmdPool;
				currentBuffer = s_UploadContext->GraphicsCmdBuffer;
				currentFence = s_UploadContext->GraphicsFence;
				currentQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				break;
			}
			case SubmitType::SUBMIT_TYPE_TRANSFER_GRAPHICS:
			case SubmitType::SUBMIT_TYPE_TRANSFER_COMPUTE:
			{
				currentPool = s_UploadContext->TransferCmdPool;
				currentBuffer = s_UploadContext->TransferCmdBuffer;
				currentFence = s_UploadContext->TransferFence;
				currentQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.TransferQueue;
				break;
			}
			case SubmitType::SUBMIT_TYPE_COMPUTE_GRAPHICS:
			case SubmitType::SUBMIT_TYPE_COMPUTE_TRANSFER:
			{
				currentPool = s_UploadContext->ComputeCmdPool;
				currentBuffer = s_UploadContext->ComputeCmdBuffer;
				currentFence = s_UploadContext->ComputeFence;
				currentQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.ComputeQueue;
				break;
			}
			default:
			{
				currentPool = s_UploadContext->GraphicsCmdPool;
				currentBuffer = s_UploadContext->GraphicsCmdBuffer;
				currentFence = s_UploadContext->GraphicsFence;
				currentQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				PX_CORE_ERROR("This submit is only allowed for QOFT commands. Use VulkanCommandControl::ImmidiateSubmitOwnershipTransfer otherwise! Falling back to graphics queue");
			}

		}
		VkCommandPool targetPool = VK_NULL_HANDLE;
		VkCommandBuffer targetBuffer = VK_NULL_HANDLE;
		VkQueue targetQueue = VK_NULL_HANDLE;
		VkFence targetFence = VK_NULL_HANDLE;
		switch (submitType)
		{
			case SubmitType::SUBMIT_TYPE_TRANSFER_GRAPHICS:
			case SubmitType::SUBMIT_TYPE_COMPUTE_GRAPHICS:
			{
				targetPool = s_UploadContext->GraphicsCmdPool;
				targetBuffer = s_UploadContext->GraphicsCmdBuffer;
				targetFence = s_UploadContext->GraphicsFence;
				targetQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				break;
			}
			case SubmitType::SUBMIT_TYPE_GRAPHICS_TRANSFER:
			case SubmitType::SUBMIT_TYPE_COMPUTE_TRANSFER:
			{
				targetPool = s_UploadContext->TransferCmdPool;
				targetBuffer = s_UploadContext->TransferCmdBuffer;
				targetFence = s_UploadContext->TransferFence;
				targetQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.TransferQueue;
				break;
			}
			case SubmitType::SUBMIT_TYPE_GRAPHICS_COMPUTE:
			case SubmitType::SUBMIT_TYPE_TRANSFER_COMPUTE:
			{
				targetPool = s_UploadContext->ComputeCmdPool;
				targetBuffer = s_UploadContext->ComputeCmdBuffer;
				targetFence = s_UploadContext->TransferFence;
				targetQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.ComputeQueue;
				break;
			}
			default:
			{
				targetPool = s_UploadContext->GraphicsCmdPool;
				targetBuffer = s_UploadContext->GraphicsCmdBuffer;
				targetFence = s_UploadContext->ComputeFence;
				targetQueue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				PX_CORE_ERROR("This submit is only allowed for QOFT commands. Use VulkanCommandControl::ImmidiateSubmitOwnershipTransfer otherwise! Falling back to graphics queue");
			}
		}
		if (!currentPool || !targetPool || !currentBuffer || !targetBuffer || !currentQueue || !targetQueue)
		{
			PX_CORE_ASSERT(true, "Something went wrong!");
			return;
		}

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		VkSemaphore ownershipSemaphore;
		VkSemaphoreCreateInfo semaInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		semaInfo.flags = VK_SEMAPHORE_TYPE_BINARY;
		PX_CORE_VK_ASSERT(vkCreateSemaphore(device, &semaInfo, nullptr, &ownershipSemaphore), VK_SUCCESS, "Failed to create Ownership Transfer Semaphore!");

		VkCommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBeginInfo.pInheritanceInfo = nullptr;

		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(currentBuffer, &cmdBeginInfo), VK_SUCCESS, "Failed to begin immidiate submit cmd buffer!");
		releaseFunction(currentBuffer);
		PX_CORE_VK_ASSERT(vkEndCommandBuffer(currentBuffer), VK_SUCCESS, "Failed to end cmd buffer!");
		

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;

		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &ownershipSemaphore;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &currentBuffer;
		PX_CORE_VK_ASSERT(vkQueueSubmit(currentQueue, 1, &submitInfo, currentFence), VK_SUCCESS, "Failed to submit cmd buffer!");
		

		
		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(targetBuffer, &cmdBeginInfo), VK_SUCCESS, "Failed to begin immidate submit cmd buffer!");
		acquireFunction(targetBuffer);
		PX_CORE_VK_ASSERT(vkEndCommandBuffer(targetBuffer), VK_SUCCESS, "Failed to end cmd buffer!");
		
		
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &ownershipSemaphore;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
		submitInfo.pCommandBuffers = &targetBuffer;
		PX_CORE_VK_ASSERT(vkQueueSubmit(targetQueue, 1, &submitInfo, targetFence), VK_SUCCESS, "Failed to submit cmd buffer!");
		
		vkWaitForFences(device, 1, &currentFence, VK_TRUE, UINT64_MAX);
		vkWaitForFences(device, 1, &targetFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &currentFence);
		vkResetFences(device, 1, &targetFence);

		vkDestroySemaphore(device, ownershipSemaphore, nullptr);

		PX_CORE_VK_ASSERT(vkResetCommandPool(device, currentPool, 0), VK_SUCCESS, "Failed to reset current command pool!");
		PX_CORE_VK_ASSERT(vkResetCommandPool(device, targetPool, 0), VK_SUCCESS, "Failed to reset target command pool!");
	}

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
		VkFence fence = VK_NULL_HANDLE;
		switch(submitType)
		{
			case SubmitType::SUBMIT_TYPE_GRAPHICS_GRAPHICS:
			{
				pool = s_UploadContext->GraphicsCmdPool;
				buffer = s_UploadContext->GraphicsCmdBuffer;
				fence = s_UploadContext->GraphicsFence;
				queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				break;
			}
			case SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER:
			{
				pool = s_UploadContext->TransferCmdPool;
				buffer = s_UploadContext->TransferCmdBuffer;
				queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.TransferQueue;
				fence = s_UploadContext->TransferFence;				
				break;
			}
			case SubmitType::SUBMIT_TYPE_COMPUTE_COMPUTE:
			{
				pool = s_UploadContext->ComputeCmdPool;
				buffer = s_UploadContext->ComputeCmdBuffer;
				queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.ComputeQueue;
				break;
			}
			default:
			{
				pool = s_UploadContext->GraphicsCmdPool;
				buffer = s_UploadContext->GraphicsCmdBuffer;
				fence = s_UploadContext->ComputeFence;
				queue = VulkanContext::GetDevice()->GetQueueFamilies().Queues.GraphicsQueue;
				PX_CORE_ERROR("This submit is only allowed for queue internal commands. Use VulkanCommandControl::ImmidiateSubmitOwnershipTransfer otherwise! Falling back to graphics queue");
			}
		}
		if (!pool || !buffer || !queue)
		{
			PX_CORE_ASSERT(true, "Upload command type undefined!");
			return;
		}

		VkCommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBeginInfo.pInheritanceInfo = nullptr;

		PX_CORE_VK_ASSERT(vkBeginCommandBuffer(buffer, &cmdBeginInfo), VK_SUCCESS, "Failed to begin immidate submit cmd buffer!");
		function(buffer);
		PX_CORE_VK_ASSERT(vkEndCommandBuffer(buffer), VK_SUCCESS, "Failed to end cmd buffer!");
				

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;

		
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;
		PX_CORE_VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence), VK_SUCCESS, "Failed to submit cmd buffer!");
		
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &fence);

		PX_CORE_VK_ASSERT(vkResetCommandPool(device, pool, 0), VK_SUCCESS, "Failed to reset command pool!");
	}

	Ref<UploadContext> VulkanCommandControl::CreateUploadContext()
	{
		PX_CORE_INFO("VulkanCommandControl::CreateUploadContext: Creating...");

		//TODO: Overhaul and align with different Queue setups!
		s_UploadContext = CreateRef<UploadContext>();
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		auto& families = VulkanContext::GetDevice()->GetQueueFamilies();

		VkFenceCreateInfo createFenceInfo{};
		createFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createFenceInfo.flags = 0;

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		VkCommandBufferAllocateInfo bufferAllocateInfo{};
		bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocateInfo.pNext = nullptr;
		bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		bufferAllocateInfo.commandBufferCount = 1;
		// Graphics
		{
			poolInfo.queueFamilyIndex = families.GraphicsFamilyIndex;
			PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &poolInfo, nullptr, &s_UploadContext->GraphicsCmdPool), VK_SUCCESS, "Failed to create graphics CommandPool!");
			
			bufferAllocateInfo.commandPool = s_UploadContext->GraphicsCmdPool;
			PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &bufferAllocateInfo, &s_UploadContext->GraphicsCmdBuffer), VK_SUCCESS, "Failed to create graphics CommandBuffer!");

			PX_CORE_VK_ASSERT(vkCreateFence(device, &createFenceInfo, nullptr, &s_UploadContext->GraphicsFence), VK_SUCCESS, "Failed to create upload fence!");
		}
		// Transfer
		{
			poolInfo.queueFamilyIndex = families.TransferFamilyIndex;
			PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &poolInfo, nullptr, &s_UploadContext->TransferCmdPool), VK_SUCCESS, "Failed to create transfer CommandPool!");

			bufferAllocateInfo.commandPool = s_UploadContext->TransferCmdPool;
			PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &bufferAllocateInfo, &s_UploadContext->TransferCmdBuffer), VK_SUCCESS, "Failed to create transfer CommandBuffer!");

			PX_CORE_VK_ASSERT(vkCreateFence(device, &createFenceInfo, nullptr, &s_UploadContext->TransferFence), VK_SUCCESS, "Failed to create upload fence!");
		}
		// Compute
		{			
			poolInfo.queueFamilyIndex = families.ComputeFamilyIndex;
			PX_CORE_VK_ASSERT(vkCreateCommandPool(device, &poolInfo, nullptr, &s_UploadContext->ComputeCmdPool), VK_SUCCESS, "Failed to create graphics compute CommandPool!");

			bufferAllocateInfo.commandPool = s_UploadContext->ComputeCmdPool;
			PX_CORE_VK_ASSERT(vkAllocateCommandBuffers(device, &bufferAllocateInfo, &s_UploadContext->ComputeCmdBuffer), VK_SUCCESS, "Failed to create compute CommandBuffer!");

			PX_CORE_VK_ASSERT(vkCreateFence(device, &createFenceInfo, nullptr, &s_UploadContext->ComputeFence), VK_SUCCESS, "Failed to create upload fence!");
		}

		s_CommandControlState.IsInitialized = true;

		PX_CORE_INFO("VulkanCommandControl::CreateUploadContext: Completed.");
		return s_UploadContext;
	}

	VulkanCommandControl::~VulkanCommandControl()
	{
		Destroy();
	}

	void VulkanCommandControl::Destroy()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		VkFence fences[] = {s_UploadContext->GraphicsFence, s_UploadContext->TransferFence, s_UploadContext->ComputeFence };
		vkWaitForFences(device, 3, fences, VK_TRUE, UINT64_MAX);
			

		vkDestroyCommandPool(device, s_UploadContext->GraphicsCmdPool, nullptr);
		s_UploadContext->GraphicsCmdPool = VK_NULL_HANDLE;
		s_UploadContext->GraphicsCmdBuffer = VK_NULL_HANDLE;
		vkDestroyCommandPool(device, s_UploadContext->TransferCmdPool, nullptr);
		s_UploadContext->TransferCmdPool = VK_NULL_HANDLE;
		s_UploadContext->TransferCmdBuffer = VK_NULL_HANDLE;
		vkDestroyCommandPool(device, s_UploadContext->ComputeCmdPool, nullptr);
		s_UploadContext->ComputeCmdPool = VK_NULL_HANDLE;
		s_UploadContext->ComputeCmdBuffer = VK_NULL_HANDLE;
				

		vkDestroyFence(device, s_UploadContext->GraphicsFence, nullptr);
		s_UploadContext->GraphicsFence = VK_NULL_HANDLE;
		vkDestroyFence(device, s_UploadContext->TransferFence, nullptr);
		s_UploadContext->TransferFence = VK_NULL_HANDLE;
		vkDestroyFence(device, s_UploadContext->ComputeFence, nullptr);
		s_UploadContext->ComputeFence = VK_NULL_HANDLE;
	}
}
