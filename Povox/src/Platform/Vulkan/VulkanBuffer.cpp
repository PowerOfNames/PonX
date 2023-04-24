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
		PX_CORE_ASSERT(specs.Size > 0, "A size needs to be defined!");
		m_Allocation = CreateAllocation(specs.Size, VulkanUtils::GetVulkanBufferUsage(specs.Usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanUtils::GetVmaUsage(specs.MemUsage));

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT bufInfo{};
		bufInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		bufInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		bufInfo.objectHandle = (uint64_t)m_Allocation.Buffer;
		bufInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), bufInfo);
#endif // DEBUG

		if(specs.Data != nullptr)
			SetData(specs.Data, specs.Size);
// 
// 		VulkanContext::SubmitResourceFree([=]()
// 			{
// 				vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Allocation.Buffer, m_Allocation.Allocation);
// 			});
	}

	void VulkanBuffer::Free()
	{
		PX_CORE_WARN("VulkanBuffer::Free Buffer {}", m_Specification.DebugName);

		VulkanContext::SubmitResourceFree([=]() 
			{
				PX_CORE_WARN("VulkanBuffer Destroying Buffer {}", m_Specification.DebugName);

				vmaDestroyBuffer(VulkanContext::GetAllocator(), m_Allocation.Buffer, m_Allocation.Allocation);
			});
	}

	void VulkanBuffer::SetData(void* inputData, const size_t size)
	{
		if(m_Allocation.Buffer == VK_NULL_HANDLE)
			m_Allocation = CreateAllocation(m_Specification.Size, VulkanUtils::GetVulkanBufferUsage(m_Specification.Usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanUtils::GetVmaUsage(m_Specification.MemUsage));
		
		m_Specification.Size = size;
		m_Specification.Data = inputData;
		AllocatedBuffer stagingBuffer = CreateAllocation(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		VmaAllocator allocator = VulkanContext::GetAllocator();
		void* data;
		vmaMapMemory(allocator, stagingBuffer.Allocation, &data);
		memcpy(data, inputData, size);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);


		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = size;

				vkCmdCopyBuffer(cmd, stagingBuffer.Buffer, m_Allocation.Buffer, 1, &copyRegion);
			});

		vmaDestroyBuffer(allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

	AllocatedBuffer VulkanBuffer::CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, std::string debugName)
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

}
