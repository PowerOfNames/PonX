#include "pxpch.h"
#include "VulkanImage2D.h"

#include "Platform/Vulkan/VulkanCommands.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanUtilities.h"


#include <stb_image.h>


namespace Povox {


	VulkanImage2D::VulkanImage2D(const ImageSpecification& spec)
		: m_Specification(spec)
	{		
		CreateImage();
		CreateImageView();
		if (m_Specification.DedicatedSampler)
			CreateSampler();
		CreateDescriptorInfo();

		if(m_Specification.Usages.ContainsUsage(ImageUsage::SAMPLED))
		{			
			if(m_Specification.CreateDescriptorOnInit)
				CreateDescriptorSet();
		}
	}

	VulkanImage2D::VulkanImage2D(uint32_t width, uint32_t height, uint32_t channels)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.ChannelCount = channels;
		m_Specification.Format = ImageFormat::RGBA8;
		m_Specification.Tiling = ImageTiling::LINEAR;
		m_Specification.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		m_Specification.Usages = { ImageUsage::COLOR_ATTACHMENT, ImageUsage::SAMPLED, ImageUsage::COPY_SRC, ImageUsage::COPY_DST };

		CreateImage();
		CreateImageView();
		if (m_Specification.DedicatedSampler)
			CreateSampler();
		CreateDescriptorInfo();
		CreateDescriptorSet();
	}

	void VulkanImage2D::Free()
	{
		PX_CORE_WARN("VulkanImage2D::Free Image {}", m_Specification.DebugName);

		VulkanContext::SubmitResourceFree([=]()
		{
			VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
			PX_CORE_WARN("VulkanImage2D::Destroying Image {}", m_Specification.DebugName);

			if (m_Sampler && m_OwnsSampler)
			{
				vkDestroySampler(device, m_Sampler, nullptr);
				m_Sampler = VK_NULL_HANDLE;
			}
			if (m_View)
			{
				vkDestroyImageView(device, m_View, nullptr);
				m_View = VK_NULL_HANDLE;
			}
			if (m_Allocation.Image)
			{
				vmaDestroyImage(VulkanContext::GetAllocator(), m_Allocation.Image, m_Allocation.Allocation);
				m_Allocation.Image = VK_NULL_HANDLE;
				m_Allocation.Allocation = VK_NULL_HANDLE;
			}

		});
		
	}

	AllocatedImage VulkanImage2D::CreateAllocation(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, VkImageLayout initialLayout, QueueFamilyOwnership ownership, std::string debugName)
	{
		VkImageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.extent = extent;
		info.format = format;

		info.arrayLayers = 1;
		info.mipLevels = 1;
		info.tiling = tiling;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.usage = usage;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.flags = 0;

 		auto& families = VulkanContext::GetDevice()->GetQueueFamilies();
		if (families.TransferExclusive())
		{
			if (families.FullyExclusive())
			{
				info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				info.pQueueFamilyIndices = nullptr;
				info.queueFamilyIndexCount = 0;				
			}
			else
			{
				info.sharingMode = VK_SHARING_MODE_CONCURRENT;
				uint32_t indices[] = { families.GraphicsFamilyIndex, families.TransferFamilyIndex };
				info.pQueueFamilyIndices = indices;
				info.queueFamilyIndexCount = 2;
			}
		}
		else
		{
			info.sharingMode = VK_SHARING_MODE_CONCURRENT;
			uint32_t indices[] = { families.GraphicsFamilyIndex, families.TransferFamilyIndex, families.ComputeFamilyIndex };
			info.pQueueFamilyIndices = indices;
			info.queueFamilyIndexCount = 3;
		}	
		
		VmaAllocationCreateInfo allocationInfo{};
		allocationInfo.usage = memUsage;
		if (allocationInfo.usage == VMA_MEMORY_USAGE_GPU_ONLY)
			allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


		//allocation needs to be cleaned up later
		AllocatedImage output;
		PX_CORE_VK_ASSERT(vmaCreateImage(VulkanContext::GetAllocator(), &info, &allocationInfo, &output.Image, &output.Allocation, nullptr), VK_SUCCESS, "Failed to create Image!");
		
#ifdef PX_DEBUG
		if(!debugName.empty())
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
			nameInfo.objectHandle = (uint64_t)output.Image;
			nameInfo.pObjectName = debugName.c_str();
			NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
		}
#endif // DEBUG
		
		return output;
	}


	// TODO: image layout transitions should happen as often as possible in subpasses!
	/**
	 * Will automatically switch ownership to QFO_TRANSFER if finalLayout contains LAYOUT_TRANSFER_*_
	 */
	void VulkanImage2D::TransitionImageLayout(
		VkImageLayout initialLayout, VkImageLayout finalLayout, 
		VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcMask, 
		VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstMask)
	{
		PX_PROFILE_FUNCTION();

		QueueFamilyOwnership originalOwnership = m_Ownership;

		auto& queueFamilies = VulkanContext::GetDevice()->GetQueueFamilies();
		uint32_t currentIndex;
		VulkanCommandControl::SubmitType submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_UNDEFINED;
		QueueFamilyOwnership targetOwner = QueueFamilyOwnership::QFO_UNDEFINED;
		/* If the image layout currently is in a transfer layout, it was target to a transfer command OR will likely switch from SRD to DST or vice versa
		*  If the final layout is a transfer layout, it will be target to a transfer command and therefore always transfers ownership to TransferQueue, if its not already there and transfer is not an exclusive queue
		*  For now, when changing the layout from transfer to something else, it will end up in the graphics queue and a separate ownership transfer is needed when the image is planned to be used in a different queue
		*/
		switch (m_Ownership)
		{
			case QueueFamilyOwnership::QFO_UNDEFINED:
			{
				currentIndex = VK_QUEUE_FAMILY_IGNORED;
				
				if (VulkanUtils::IsGraphicsStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_GRAPHICS;
					targetOwner = QueueFamilyOwnership::QFO_GRAPHICS;
					break;
				}
				if (VulkanUtils::IsComputeStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_COMPUTE;
					targetOwner = QueueFamilyOwnership::QFO_COMPUTE;

					break;
				}
				if (VulkanUtils::IsTransferStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER;
					targetOwner = QueueFamilyOwnership::QFO_TRANSFER;

					break;
				}

				PX_CORE_ASSERT(true, "dstStage not caught yet!");
				break;
			}
			case QueueFamilyOwnership::QFO_GRAPHICS:
			{
				currentIndex = queueFamilies.GraphicsFamilyIndex;
				if (VulkanUtils::IsGraphicsStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_GRAPHICS;
					targetOwner = QueueFamilyOwnership::QFO_GRAPHICS;

					break;
				}
				if (VulkanUtils::IsComputeStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_COMPUTE;
					targetOwner = QueueFamilyOwnership::QFO_COMPUTE;

					break;
				}
				if (VulkanUtils::IsTransferStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_TRANSFER;
					targetOwner = QueueFamilyOwnership::QFO_TRANSFER;

					break;
				}

				PX_CORE_ASSERT(true, "dstStage not caught yet!");
				break;				
			}
			case QueueFamilyOwnership::QFO_TRANSFER:
			{
				currentIndex = queueFamilies.TransferFamilyIndex;
				if (VulkanUtils::IsGraphicsStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_GRAPHICS;
					targetOwner = QueueFamilyOwnership::QFO_GRAPHICS;

					break;
				}
				if (VulkanUtils::IsComputeStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_COMPUTE;
					targetOwner = QueueFamilyOwnership::QFO_COMPUTE;

					break;
				}
				if (VulkanUtils::IsTransferStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER;
					targetOwner = QueueFamilyOwnership::QFO_TRANSFER;

					break;
				}

				PX_CORE_ASSERT(true, "dstStage not caught yet!");
				break;				
			}
			case QueueFamilyOwnership::QFO_COMPUTE:
			{
				currentIndex = queueFamilies.ComputeFamilyIndex;
				if (VulkanUtils::IsGraphicsStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_GRAPHICS;
					targetOwner = QueueFamilyOwnership::QFO_GRAPHICS;

					break;
				}
				if (VulkanUtils::IsComputeStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_COMPUTE;
					targetOwner = QueueFamilyOwnership::QFO_COMPUTE;

					break;
				}
				if (VulkanUtils::IsTransferStage(dstStage))
				{
					submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_TRANSFER;
					targetOwner = QueueFamilyOwnership::QFO_TRANSFER;

					break;
				}

				PX_CORE_ASSERT(true, "dstStage not caught yet!");
				break;				
			}
			default:
			{
				PX_CORE_ASSERT(true, "QueueFamilyOwnership not caught!");
			}
			break;
		}			
		
		switch (submitType)
		{
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_COMPUTE:
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_GRAPHICS:
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER:
			{
				VulkanCommandControl::ImmidiateSubmit(submitType, [=](VkCommandBuffer cmd)
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = currentIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.srcStageMask = srcStage;
						barrier.srcAccessMask = srcMask;
						barrier.dstStageMask = dstStage;
						barrier.dstAccessMask = dstMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(cmd, &dependency);
					});
				m_CurrentLayout = finalLayout;
				m_Ownership = targetOwner;
				break;
			}
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_GRAPHICS:
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_GRAPHICS:
			{	
				VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(submitType
					, [=](VkCommandBuffer releaseCmd)				
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = queueFamilies.GraphicsFamilyIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.srcStageMask = srcStage;
						barrier.srcAccessMask = srcMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(releaseCmd, &dependency);
					}, [=](VkCommandBuffer acquireCmd) 
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = queueFamilies.GraphicsFamilyIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.dstStageMask = dstStage;
						barrier.dstAccessMask = dstMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(acquireCmd, &dependency);
					});
				m_Ownership = QueueFamilyOwnership::QFO_GRAPHICS;
				break;
			}
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_TRANSFER:
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_TRANSFER:
			{
				VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(submitType
					, [=](VkCommandBuffer releaseCmd)
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = queueFamilies.TransferFamilyIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.srcStageMask = srcStage;
						barrier.srcAccessMask = srcMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(releaseCmd, &dependency);
					}, [=](VkCommandBuffer acquireCmd)
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = queueFamilies.TransferFamilyIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.dstStageMask = dstStage;
						barrier.dstAccessMask = dstMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(acquireCmd, &dependency);
					});
				m_Ownership = QueueFamilyOwnership::QFO_TRANSFER;
				break;
			}
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_COMPUTE:
			case VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_COMPUTE:
			{
				VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(submitType, [=](VkCommandBuffer releaseCmd)
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = queueFamilies.ComputeFamilyIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.srcStageMask = srcStage;
						barrier.srcAccessMask = srcMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(releaseCmd, &dependency);
					}, [=](VkCommandBuffer acquireCmd)
					{
						VkImageMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
						barrier.image = m_Allocation.Image;
						barrier.oldLayout = initialLayout;
						barrier.newLayout = finalLayout;
						barrier.srcQueueFamilyIndex = currentIndex;
						barrier.dstQueueFamilyIndex = queueFamilies.ComputeFamilyIndex;
						barrier.subresourceRange.aspectMask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.dstStageMask = dstStage;
						barrier.dstAccessMask = dstMask;

						VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR };
						dependency.imageMemoryBarrierCount = 1;
						dependency.pImageMemoryBarriers = &barrier;
						vkCmdPipelineBarrier2(acquireCmd, &dependency);
					});
				m_Ownership = QueueFamilyOwnership::QFO_COMPUTE;
				break;
			}			
		}
		m_CurrentLayout = finalLayout;
		CreateDescriptorInfo();
	}

	void VulkanImage2D::SetData(void* data)
	{
		size_t imageSize = m_Specification.Width * m_Specification.Height * m_Specification.ChannelCount;
		AllocatedBuffer stagingBuffer = VulkanBuffer::CreateAllocation(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, QueueFamilyOwnership::QFO_TRANSFER, "Image:SetData StagingBuffer");
		
		void* databuffer;
		vmaMapMemory(VulkanContext::GetAllocator(), stagingBuffer.Allocation, &databuffer);
		memcpy(databuffer, data, static_cast<size_t>(imageSize));
		vmaUnmapMemory(VulkanContext::GetAllocator(), stagingBuffer.Allocation);

		TransitionImageLayout(
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_2_NONE, 0,
			VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferImageHeight = 0;
				region.bufferRowLength = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { static_cast<uint32_t>(m_Specification.Width), static_cast<uint32_t>(m_Specification.Height), 1 };

				vkCmdCopyBufferToImage(cmd, stagingBuffer.Buffer, m_Allocation.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			});

		VkImageLayout finalLayout;
		if (m_Specification.Usages.ContainsUsage(ImageUsage::SAMPLED))
		{
			finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		TransitionImageLayout(
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout,
			VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);		

		vmaDestroyBuffer(VulkanContext::GetAllocator(), stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

	int VulkanImage2D::ReadPixel(int posX, int posY)
	{
		// TODO: Check if the image I want to read from is readable (was downloaded from GPU after rendering)
		size_t imageSize = m_Specification.Width * m_Specification.Height * m_Specification.ChannelCount;
		AllocatedBuffer stagingBuffer = VulkanBuffer::CreateAllocation(imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY, QueueFamilyOwnership::QFO_TRANSFER, "ReadPixel StaginBuffer");

		TransitionImageLayout(
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_WRITE_BIT
		);
		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferImageHeight = 0;
				region.bufferRowLength = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { static_cast<uint32_t>(m_Specification.Width), static_cast<uint32_t>(m_Specification.Height), 1 };

				vkCmdCopyImageToBuffer(cmd, m_Allocation.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.Buffer, 1, &region);
			});
		VkImageLayout newLayout;
		if (m_Specification.Usages.ContainsUsage(ImageUsage::SAMPLED))
		{
			newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		TransitionImageLayout(
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newLayout,
			VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_WRITE_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT
		);
		
		void* dataBuffer;
		vmaMapMemory(VulkanContext::GetAllocator(), stagingBuffer.Allocation, &dataBuffer);
		int* data = static_cast<int*>(dataBuffer);
		vmaUnmapMemory(VulkanContext::GetAllocator(), stagingBuffer.Allocation);
		vmaDestroyBuffer(VulkanContext::GetAllocator(), stagingBuffer.Buffer, stagingBuffer.Allocation);

		return data[m_Specification.Width * posY + posX];
	}

	void VulkanImage2D::CreateImage()
	{
		m_Ownership = QueueFamilyOwnership::QFO_UNDEFINED;
		m_Allocation = CreateAllocation({ m_Specification.Width, m_Specification.Height, 1 }, VulkanUtils::GetVulkanImageFormat(m_Specification.Format), VulkanUtils::GetVulkanTiling(m_Specification.Tiling),
			VulkanUtils::GetVulkanImageUsages(m_Specification.Usages), VulkanUtils::GetVmaUsage(m_Specification.Memory), VK_IMAGE_LAYOUT_UNDEFINED, m_Ownership, m_Specification.DebugName);

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
		nameInfo.objectHandle = (uint64_t)m_Allocation.Image;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

		PX_CORE_INFO("VulkanImage2D::CreateImage: Created Image {} with extent '{}, {}'", m_Specification.DebugName, m_Specification.Width, m_Specification.Height);
	}

	void VulkanImage2D::CreateImageView()
	{
		VkImageAspectFlags mask = VulkanUtils::GetAspectFlagsFromUsages(m_Specification.Usages);

		VkImageViewCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_Allocation.Image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VulkanUtils::GetVulkanImageFormat(m_Specification.Format);

		info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		info.subresourceRange.aspectMask = mask;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		PX_CORE_VK_ASSERT(vkCreateImageView(VulkanContext::GetDevice()->GetVulkanDevice(), &info, nullptr, &m_View), VK_SUCCESS, "Failed to create image view!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		nameInfo.objectHandle = (uint64_t)m_View;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG
	}


	VkDescriptorImageInfo VulkanImage2D::CreateDescriptorInfo()
	{
		VkDescriptorImageInfo descInfo{};
		descInfo.imageLayout = m_CurrentLayout;
		descInfo.imageView = m_View;
		descInfo.sampler = nullptr;
		if(m_Specification.DedicatedSampler)
			descInfo.sampler = m_Sampler;

		m_DescriptorInfo = descInfo;
		return m_DescriptorInfo;

	}

	void VulkanImage2D::CreateSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(VulkanContext::GetDevice()->GetPhysicalDevice(), &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.minLod = 0.0f;
		PX_CORE_VK_ASSERT(vkCreateSampler(VulkanContext::GetDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_Sampler), VK_SUCCESS, "Failed to create texture sampler!");

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_SAMPLER;
		nameInfo.objectHandle = (uint64_t)m_Sampler;
		nameInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), nameInfo);
#endif // DEBUG

		m_OwnsSampler = true;
	}
	
	void VulkanImage2D::CreateDescriptorSet()
	{
		m_Binding.binding = 0;
		m_Binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		if (m_Specification.DedicatedSampler)
			m_Binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			
		m_Binding.descriptorCount = 1;
		m_Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_Binding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> textureBindings = { m_Binding };

		m_DescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorInfo.imageView = m_View;
		m_DescriptorInfo.sampler = m_Sampler;

		VulkanDescriptorBuilder builder = VulkanDescriptorBuilder::Begin(VulkanContext::GetDescriptorLayoutCache(), VulkanContext::GetDescriptorAllocator());
		builder.BindImage(m_Binding, &m_DescriptorInfo);
		builder.Build(m_DescriptorSet, "ImageDS");
	}

// ----------------- Texture2D -----------------


	VulkanTexture2D::VulkanTexture2D(uint32_t width, uint32_t height, uint32_t channels, const std::string& debugName)
	{
		m_Path = "No Path";

		ImageSpecification specs{};
		specs.DebugName = debugName;
		specs.Width = width;
		specs.Height = height;
		specs.ChannelCount = channels;
		specs.ChannelCount = 4;
		specs.Format = ImageFormat::RGBA8;
		specs.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.MipLevels = 1;
		specs.Tiling = ImageTiling::LINEAR;
		specs.Usages = { ImageUsage::SAMPLED, ImageUsage::COPY_DST };
		m_Image = CreateRef<VulkanImage2D>(specs);

		PX_CORE_TRACE("Texture width : '{0}', height '{1}', RID: {2}", width, height, std::to_string(m_Handle).c_str());
	}

	VulkanTexture2D::VulkanTexture2D(const std::string& path, const std::string& debugName)
	{
		m_Path = path;
		int width, height, channels;
		stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		PX_CORE_ASSERT(pixels, "Failed to load texture!");
		PX_CORE_ASSERT(width > 0 && height > 0 && channels > 0, "Texture loading failed with insufficient dimensionality!");
		
		ImageSpecification specs{};
		specs.DebugName = debugName;
		specs.Width = static_cast<uint32_t>(width);
		specs.Height = static_cast<uint32_t>(height);
		//specs.ChannelCount = static_cast<uint32_t>(channels);
		specs.ChannelCount = 4;
		/*switch (channels)
		{
			case 1: specs.Format = ImageFormat::RED_INTEGER; break;
			case 2: specs.Format = ImageFormat::RG8; break;
			case 3: specs.Format = ImageFormat::RGB8; break;
			case 4: specs.Format = ImageFormat::RGBA8; break;
			default: PX_CORE_ASSERT(true, "Unexpected channel count!");
		}*/
		specs.Format = ImageFormat::RGBA8;
		specs.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.MipLevels = 1;
		specs.Tiling = ImageTiling::LINEAR;
		specs.Usages = { ImageUsage::SAMPLED, ImageUsage::COPY_DST };
		m_Image = CreateRef<VulkanImage2D>(specs);


		PX_CORE_TRACE("Texture width : '{0}', height '{1}', RID: {2}", width, height, std::to_string(m_Handle).c_str());

		SetData(pixels);
		stbi_image_free(pixels);
	}

	void VulkanTexture2D::Free()
	{
		m_Image->Free();
		m_Handle = 0;
	}

	void VulkanTexture2D::SetData(void* data)
	{
		m_Image->SetData(data);
	}
}
