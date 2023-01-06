#pragma once
#include "VulkanDevice.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Renderable.h"


#include "vk_mem_alloc.h"

namespace Povox {

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
		virtual inline BufferSpecification& GetSpecification() override { return m_Specification; }

		inline const AllocatedBuffer& GetAllocation() const { return m_Allocation; }
		inline AllocatedBuffer& GetAllocation() { return m_Allocation; }

		static AllocatedBuffer CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);

	private:
		BufferSpecification m_Specification;
		AllocatedBuffer m_Allocation{};
	};
}
