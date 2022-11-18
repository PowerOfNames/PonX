#pragma once
#include "VulkanDevice.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Renderable.h"


#include "vk_mem_alloc.h"

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

	struct AllocatedBuffer
	{
		VkBuffer Buffer = VK_NULL_HANDLE;
		VmaAllocation Allocation = nullptr;
	};
	

	class VulkanBuffer : public Buffer
	{
	public:
		VulkanBuffer(const BufferSpecification& specs);
		~VulkanBuffer();

		virtual void SetData(void* inputData, size_t size) override;
		//virtual void SetLayout(const BufferLayout& vertexLayout) override { m_Specification.VertexLayout = vertexLayout; }
		virtual BufferSpecification& GetSpecification() override { return m_Specification; }

		const AllocatedBuffer& GetAllocation() { return m_Allocation; }

		static AllocatedBuffer CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);

	private:
		BufferSpecification m_Specification;
		AllocatedBuffer m_Allocation{};
	};
}
