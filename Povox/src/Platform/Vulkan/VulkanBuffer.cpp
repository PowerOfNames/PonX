#include "pxpch.h"

#include "VulkanBuffer.h"
#include "VulkanCommands.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"


namespace Povox {

	namespace VulkanUtils {

		VkBufferUsageFlags GetVulkanBufferUsage(BufferUsage usage)
		{
			VkBufferUsageFlags flags = 0;
			if ((usage & BufferUsage::VERTEX_BUFFER) == BufferUsage::VERTEX_BUFFER)
				flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			if ((usage & BufferUsage::INDEX_BUFFER_32) == BufferUsage::INDEX_BUFFER_32)
				flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			if ((usage & BufferUsage::INDEX_BUFFER_64) == BufferUsage::INDEX_BUFFER_64)
				flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			if ((usage & BufferUsage::UNIFORM_BUFFER) == BufferUsage::UNIFORM_BUFFER)
				flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			if ((usage & BufferUsage::UNIFORM_BUFFER_DYNAMIC) == BufferUsage::UNIFORM_BUFFER_DYNAMIC)
				flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			if ((usage & BufferUsage::STORAGE_BUFFER) == BufferUsage::STORAGE_BUFFER)
				flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			if ((usage & BufferUsage::STORAGE_BUFFER_DYNAMIC) == BufferUsage::STORAGE_BUFFER_DYNAMIC)
				flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			

			return flags;
		}
	}

	VulkanBuffer::VulkanBuffer(const BufferSpecification& specs)
		: m_Specification(specs)
	{
		PX_CORE_ASSERT(specs.Size > 0, "A size needs to be defined!");
		m_Size = specs.Size;

		m_Ownership = QueueFamilyOwnership::QFO_GRAPHICS;
		m_Allocation = CreateAllocation(m_Size, VulkanUtils::GetVulkanBufferUsage(specs.Usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanUtils::GetVmaUsage(specs.MemUsage), m_Ownership, specs.DebugName+ "Allocation");
		m_Staging = CreateAllocation(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, QueueFamilyOwnership::QFO_TRANSFER, specs.DebugName + "Staging");

		CreateDescriptorInfo();

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT bufInfo{};
		bufInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		bufInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		bufInfo.objectHandle = (uint64_t)m_Allocation.Buffer;
		bufInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), bufInfo);
#endif // DEBUG

//		// Differentiate between Buffers that live during the 'whole' runtime and time based buffers -> depending on that the resource 
//		// free call needs to take place here or Free() needs to be called
// 		VulkanContext::SubmitResourceFree([=]()
// 			{
//				VmaAllocator allocator = VulkanContext::GetAllocator();
// 
// 				vmaDestroyBuffer(allocator, m_Allocation.Buffer, m_Allocation.Allocation);
// 				vmaDestroyBuffer(allocator, m_Staging.Buffer, m_Staging.Allocation);
// 			});
	}

	void VulkanBuffer::Free()
	{
		PX_CORE_WARN("VulkanBuffer::Free Buffer {}", m_Specification.DebugName);

		VulkanContext::SubmitResourceFree([=]() 
			{
				PX_CORE_WARN("VulkanBuffer Destroying Buffer {}", m_Specification.DebugName);

				VmaAllocator allocator = VulkanContext::GetAllocator();

				vmaDestroyBuffer(allocator, m_Allocation.Buffer, m_Allocation.Allocation);
				vmaDestroyBuffer(allocator, m_Staging.Buffer, m_Staging.Allocation);
			});
	}

	void VulkanBuffer::UploadToGPU()
	{	
		PX_CORE_ASSERT(m_StagingMapped, "Something went wrong, staging should be mapped!");

		VmaAllocator allocator = VulkanContext::GetAllocator();
		vmaUnmapMemory(allocator, m_Staging.Allocation);
		m_StagingMapped = false;

		QueueFamilyOwnership originalOwnership = m_Ownership;

		auto& queueFamilies = VulkanContext::GetDevice()->GetQueueFamilies();
		uint32_t currentIndex;
		VulkanCommandControl::SubmitType submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_UNDEFINED;
		VulkanCommandControl::SubmitType reverseSubmitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_UNDEFINED;

		switch (m_Ownership)
		{
			case QueueFamilyOwnership::QFO_GRAPHICS:
			{
				currentIndex = queueFamilies.GraphicsFamilyIndex;
				submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_GRAPHICS_TRANSFER;
				reverseSubmitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_GRAPHICS;

				break;
			}
			case QueueFamilyOwnership::QFO_TRANSFER:
			{
				currentIndex = queueFamilies.TransferFamilyIndex;				
				submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER;
				
				break;
			}
			case QueueFamilyOwnership::QFO_COMPUTE:
			{
				currentIndex = queueFamilies.ComputeFamilyIndex;				
				submitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_COMPUTE_TRANSFER;
				reverseSubmitType = VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_COMPUTE;
				break;
			}
			default:
			{
				PX_CORE_ASSERT(true, "QueueFamilyOwnership not caught!");
				break;
			}
		}

		VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(submitType, [=](VkCommandBuffer releaseCmd)
			{
				VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
				barrier.pNext = nullptr;
				barrier.buffer = m_Allocation.Buffer;
				barrier.size = m_Size;
				barrier.offset = 0; // TODO:
				barrier.srcQueueFamilyIndex = currentIndex;
				barrier.dstQueueFamilyIndex = queueFamilies.TransferFamilyIndex;

				barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
				barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;

				VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
				dependency.bufferMemoryBarrierCount = 1;
				dependency.pBufferMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(releaseCmd, &dependency);

			}, [=](VkCommandBuffer acquireCmd)
			{
				VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
				barrier.pNext = nullptr;
				barrier.buffer = m_Allocation.Buffer;
				barrier.size = m_Size;
				barrier.offset = 0; // TODO:
				barrier.srcQueueFamilyIndex = currentIndex;
				barrier.dstQueueFamilyIndex = queueFamilies.TransferFamilyIndex;

				barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

				VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
				dependency.bufferMemoryBarrierCount = 1;
				dependency.pBufferMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(acquireCmd, &dependency);
			});
		m_Ownership = QueueFamilyOwnership::QFO_TRANSFER;

		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = m_Size;

				vkCmdCopyBuffer(cmd, m_Staging.Buffer, m_Allocation.Buffer, 1, &copyRegion);
			});

		VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(reverseSubmitType, [=](VkCommandBuffer releaseCmd)
			{
				VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
				barrier.pNext = nullptr;
				barrier.buffer = m_Allocation.Buffer;
				barrier.size = m_Size;
				barrier.offset = 0; // TODO:
				barrier.srcQueueFamilyIndex = queueFamilies.TransferFamilyIndex;
				barrier.dstQueueFamilyIndex = currentIndex;

				barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

				VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
				dependency.bufferMemoryBarrierCount = 1;
				dependency.pBufferMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(releaseCmd, &dependency);

			}, [=](VkCommandBuffer acquireCmd)
			{
				VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
				barrier.pNext = nullptr;
				barrier.buffer = m_Allocation.Buffer;
				barrier.size = m_Size;
				barrier.offset = 0; // TODO:
				barrier.srcQueueFamilyIndex = queueFamilies.TransferFamilyIndex;
				barrier.dstQueueFamilyIndex = currentIndex;


				barrier.dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

				VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
				dependency.bufferMemoryBarrierCount = 1;
				dependency.pBufferMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(acquireCmd, &dependency);
			});
		m_Ownership = originalOwnership;

		CreateDescriptorInfo();
	}

	Ref<BufferSuballocation> VulkanBuffer::GetSuballocation(size_t size)
	{
		// TODO: catch this, create new buffer and return a suballocation to this using a bufferPool
		if (m_Size - size < m_SuballocationOffset)
		{
			PX_CORE_ASSERT(true, "Requested suballocation size exceeds size left in this buffer!");
		}
		size_t paddedSize = PadSize(size);
		size_t offset = m_SuballocationOffset;
		Ref<BufferSuballocation> sub = CreateRef<BufferSuballocation>(GetPtr(), offset, paddedSize);
		m_SuballocationOffset =+ paddedSize;

		return sub;
	}

	void VulkanBuffer::TransferOwnership(QueueFamilyOwnership newOwnership, uint64_t offset, uint64_t range)
	{
		if (newOwnership == m_Ownership)
			return;

		if (m_Ownership == QueueFamilyOwnership::QFO_UNDEFINED)
		{
			m_Ownership = newOwnership;
			return;
		}

		if (range == 0)
			return;

		auto& transferConfig = VulkanCommandControl::DetermineSubmitType(m_Ownership, newOwnership);

		VkAccessFlags2 srcAccess;
		VkPipelineStageFlags2 srcStage;

		switch (m_Ownership)
		{
			case QueueFamilyOwnership::QFO_GRAPHICS:
			{
				srcAccess = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
				break;
			}
			case QueueFamilyOwnership::QFO_COMPUTE:
			{
				srcAccess = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
				break;
			}
			case QueueFamilyOwnership::QFO_TRANSFER:
			{
				srcAccess = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				break;
			}
			case QueueFamilyOwnership::QFO_UNDEFINED:
				PX_CORE_ASSERT(true, "VulkanBuffer::TransferOwnership: Undefined not allowed!");
		}
		VkAccessFlags2 dstAccess;
		VkPipelineStageFlags2 dstStage;
		switch (newOwnership)
		{
			case QueueFamilyOwnership::QFO_COMPUTE:
			{
				dstAccess = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
				dstStage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
				break;
			}
			case QueueFamilyOwnership::QFO_GRAPHICS:
			{
				dstAccess = VK_ACCESS_2_NONE;
				dstStage = VK_PIPELINE_STAGE_2_NONE;
				break;
			}
			case QueueFamilyOwnership::QFO_TRANSFER:
			{
				dstAccess = VK_ACCESS_2_MEMORY_READ_BIT;
				dstStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				break;
			}
		}

		VulkanCommandControl::ImmidiateSubmitOwnershipTransfer(transferConfig.SubmissionType
			, [=](VkCommandBuffer releaseCmd)
			{
				VkBufferMemoryBarrier2 bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
				bufBarrier.pNext = nullptr;
				bufBarrier.srcQueueFamilyIndex = transferConfig.SrcQueueIndex;
				bufBarrier.dstQueueFamilyIndex = transferConfig.DstQueueIndex;

				bufBarrier.srcAccessMask = srcAccess;
				bufBarrier.srcStageMask = srcStage;

				bufBarrier.buffer = m_Allocation.Buffer;
				bufBarrier.offset = offset;
				bufBarrier.size = range;

				VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
				dependency.pNext = nullptr;
				dependency.bufferMemoryBarrierCount = 1;
				dependency.pBufferMemoryBarriers = &bufBarrier;
				vkCmdPipelineBarrier2(releaseCmd, &dependency);
			}
			, [=](VkCommandBuffer acquireCmd)
				{
					VkBufferMemoryBarrier2 bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
					bufBarrier.pNext = nullptr;
					bufBarrier.srcQueueFamilyIndex = transferConfig.SrcQueueIndex;
					bufBarrier.dstQueueFamilyIndex = transferConfig.DstQueueIndex;

					bufBarrier.dstAccessMask = dstAccess;
					bufBarrier.dstStageMask = dstStage;

					bufBarrier.buffer = m_Allocation.Buffer;
					bufBarrier.offset = offset;
					bufBarrier.size = range;

					VkDependencyInfo dependency{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
					dependency.pNext = nullptr;
					dependency.bufferMemoryBarrierCount = 1;
					dependency.pBufferMemoryBarriers = &bufBarrier;
					vkCmdPipelineBarrier2(acquireCmd, &dependency);
				});

		m_Ownership = newOwnership;
	}

	uint32_t VulkanBuffer::GetPadding()
	{
		return VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	}

	size_t VulkanBuffer::PadSize(size_t initialSize)
	{
		size_t alignedSize = initialSize;
		uint32_t minGPUBufferOffsetAlignment = VulkanContext::GetDevice()->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
		if (minGPUBufferOffsetAlignment > 0)
		{
			alignedSize = (alignedSize + minGPUBufferOffsetAlignment - 1) & ~(minGPUBufferOffsetAlignment - 1);
		}
		return alignedSize;
	}

	void VulkanBuffer::SetData(void* inputData, const size_t size)
	{
		if (m_Specification.Size < size)
		{
			PX_CORE_ERROR("VulkanBuffer::SetData: The BufferSpecification.Size and therefore the initial buffer size is too small for this dataset!");
			return;
		}

		if (!m_StagingMapped)
		{
			VmaAllocator allocator = VulkanContext::GetAllocator();
			vmaMapMemory(allocator, m_Staging.Allocation, &m_Data);
			m_StagingMapped = true;
		}
		memcpy(m_Data, inputData, size);

		UploadToGPU();
	}
	void VulkanBuffer::SetData(void* inputData, size_t offset, size_t size)
	{
		PX_CORE_ASSERT((offset + size) <= m_Specification.Size, "Out of bounds!");


		if (!m_StagingMapped)
		{
			VmaAllocator allocator = VulkanContext::GetAllocator();
			vmaMapMemory(allocator, m_Staging.Allocation, &m_Data);
			m_StagingMapped = true;
		}
		char* data = (char*)m_Data;
		memcpy(data + offset, inputData, size);

		UploadToGPU();
	}	

	AllocatedBuffer VulkanBuffer::CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, QueueFamilyOwnership ownership, std::string debugName)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;

		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.queueFamilyIndexCount = 1;
		auto& families = VulkanContext::GetDevice()->GetQueueFamilies();
		
		// TODO: queue family can be empty if not concurrent
		switch (ownership)
		{
			case QueueFamilyOwnership::QFO_UNDEFINED:
			case QueueFamilyOwnership::QFO_GRAPHICS:
			{
				uint32_t familyIndex[] = { families.GraphicsFamilyIndex };
				bufferInfo.pQueueFamilyIndices = familyIndex;
				break;
			}
			case QueueFamilyOwnership::QFO_TRANSFER:
			{
				uint32_t familyIndex[] = { families.TransferFamilyIndex };
				bufferInfo.pQueueFamilyIndices = familyIndex;
				break;
			}
			case QueueFamilyOwnership::QFO_COMPUTE:
			{
				uint32_t familyIndex[] = { families.ComputeFamilyIndex };
				bufferInfo.pQueueFamilyIndices = familyIndex;
				break;
			}
			default:
			{
				PX_CORE_ASSERT(true, "QueueFamilyOwnership not caught!");
			}
		}

		VmaAllocationCreateInfo vmaAllocInfo{};
		//vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		//vmaAllocInfo.flags = 
		vmaAllocInfo.usage = memUsage;

		AllocatedBuffer newBuffer;
		PX_CORE_VK_ASSERT(vmaCreateBuffer(VulkanContext::GetAllocator(), &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, nullptr), VK_SUCCESS, "Failed to create Buffer!");

#ifdef PX_DEBUG
		if (!debugName.empty())
		{
			VkDebugUtilsObjectNameInfoEXT bufInfo{};
			bufInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			bufInfo.objectType = VK_OBJECT_TYPE_BUFFER;
			bufInfo.objectHandle = (uint64_t)newBuffer.Buffer;
			bufInfo.pObjectName = debugName.c_str();
			NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), bufInfo);
		}
#endif // DEBUG

		return newBuffer;
	}

	VkDescriptorBufferInfo VulkanBuffer::CreateDescriptorInfo(size_t offset /*= 0*/, size_t range /*= 0*/)
	{
		VkDescriptorBufferInfo descInfo{};
		descInfo.buffer = m_Allocation.Buffer;
		descInfo.offset = offset;
		descInfo.range = range != 0 ? range : m_Size;

		m_DescriptorInfo = descInfo;
		return m_DescriptorInfo;
	}	

}
