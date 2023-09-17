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
		m_Size = specs.Size;

		m_Allocation = CreateAllocation(m_Size, VulkanUtils::GetVulkanBufferUsage(specs.Usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VulkanUtils::GetVmaUsage(specs.MemUsage), &m_Ownership, specs.DebugName+ "Allocation");
		m_Staging = CreateAllocation(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, nullptr, specs.DebugName + "Staging");

		CreateDescriptorInfo();

#ifdef PX_DEBUG
		VkDebugUtilsObjectNameInfoEXT bufInfo{};
		bufInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		bufInfo.objectType = VK_OBJECT_TYPE_BUFFER;
		bufInfo.objectHandle = (uint64_t)m_Allocation.Buffer;
		bufInfo.pObjectName = m_Specification.DebugName.c_str();
		NameVkObject(VulkanContext::GetDevice()->GetVulkanDevice(), bufInfo);
#endif // DEBUG

//		// Differentiate between Buffers that live durign the whole runtime and time based buffers -> depending on that the resource 
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

		VulkanCommandControl::ImmidiateSubmit(VulkanCommandControl::SubmitType::SUBMIT_TYPE_TRANSFER_TRANSFER, [=](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = m_Size;

				vkCmdCopyBuffer(cmd, m_Staging.Buffer, m_Allocation.Buffer, 1, &copyRegion);
			});

		CreateDescriptorInfo();
	}

	void VulkanBuffer::SetData(void* inputData, const size_t size)
	{
		if (m_Specification.Size > size)
			PX_CORE_WARN("VulkanBuffer::SetData: The BufferSpecification size does not match the data size!");

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
	void VulkanBuffer::SetData(void* inputData, uint32_t offset, size_t size)
	{
		PX_CORE_ASSERT((offset + size) < m_Specification.Size, "Out of bounds!");
		//char* input = (char*)inputData;

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

	AllocatedBuffer VulkanBuffer::CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, QueueFamilyOwnership* ownership, std::string debugName)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.pNext = nullptr;

		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.queueFamilyIndexCount = 1;
		auto& families = VulkanContext::GetDevice()->GetQueueFamilies();
		if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT || usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
		{
			uint32_t familyIndex[] = { families.TransferFamilyIndex };
			bufferInfo.pQueueFamilyIndices = familyIndex;
			if(ownership)
				*ownership = QueueFamilyOwnership::QFO_TRANSFER;
		}
		else
		{
			uint32_t familyIndex[] = { families.TransferFamilyIndex };
			bufferInfo.pQueueFamilyIndices = familyIndex;
			if(ownership)
				*ownership = QueueFamilyOwnership::QFO_GRAPHICS;
		}
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

	VkDescriptorBufferInfo VulkanBuffer::CreateDescriptorInfo()
	{
		VkDescriptorBufferInfo descInfo{};
		descInfo.buffer = m_Allocation.Buffer;
		descInfo.offset = 0;
		descInfo.range = m_Size;

		m_DescriptorInfo = descInfo;
		return m_DescriptorInfo;
	}	

}
