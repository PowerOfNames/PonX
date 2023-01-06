#include "pxpch.h"

#include "VulkanBuffer.h"
#include "VulkanCommands.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"


namespace Povox {

	namespace VulkanUtils {

		VkBufferUsageFlags GetVulkanBufferUsage(BufferUsage usage)
		{
			switch (usage)
			{
				case BufferUsage::VERTEX_BUFFER:	return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				case BufferUsage::INDEX_BUFFER:		return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				case BufferUsage::UNIFORM_BUFFER:	return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				case BufferUsage::STORAGE_BUFFER:	return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
				case BufferUsage::UNDEFINED:		break;
			}
			PX_CORE_ASSERT(true, "BufferUsage not defined!");
			return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		}
	}

	VulkanBuffer::VulkanBuffer(const BufferSpecification& specs)
		: m_Specification(specs)
	{
		if (specs.Size <= 0)
			m_Allocation = CreateAllocation(specs.Size, VulkanUtils::GetVulkanBufferUsage(specs.Usage), VulkanUtils::GetVmaUsage(specs.MemUsage));

		if(specs.Data != nullptr)
			SetData(specs.Data, specs.Size);

		VulkanContext::SubmitResourceFree([=]()
			{
				vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Allocation.Buffer, m_Allocation.Allocation);
			});
	}

	VulkanBuffer::~VulkanBuffer()
	{
		vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Allocation.Buffer, m_Allocation.Allocation);
	}

	void VulkanBuffer::SetData(void* inputData, size_t size)
	{
		if(m_Allocation.Buffer == VK_NULL_HANDLE)
			m_Allocation = CreateAllocation(m_Specification.Size, VulkanUtils::GetVulkanBufferUsage(m_Specification.Usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanUtils::GetVmaUsage(m_Specification.MemUsage));
		
		const size_t bufferSize = size;
		m_Specification.Size = size;
		m_Specification.Data = inputData;
		AllocatedBuffer stagingBuffer = CreateAllocation(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		VmaAllocator allocator = VulkanContext::GetAllocator();
		void* data;
		vmaMapMemory(allocator, stagingBuffer.Allocation, &data);
		memcpy(data, inputData, bufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);


		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = bufferSize;

				vkCmdCopyBuffer(cmd, stagingBuffer.Buffer, m_Allocation.Buffer, 1, &copyRegion);
			});

		vmaDestroyBuffer(allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

	AllocatedBuffer VulkanBuffer::CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;

		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = memUsage;

		AllocatedBuffer newBuffer;

		PX_CORE_VK_ASSERT(vmaCreateBuffer(VulkanContext::GetAllocator(), &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, nullptr), VK_SUCCESS, "Failed to create Buffer!");

		return newBuffer;
	}

}
