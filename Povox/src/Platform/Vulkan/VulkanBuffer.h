#pragma once
#include "VulkanDevice.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Renderable.h"

#include "Platform/Vulkan/VulkanUtilities.h"

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
		virtual ~VulkanBuffer() = default;
		virtual void Free() override;

		virtual BufferHandle GetID() const override { return m_Handle; }

		virtual Ref<BufferSuballocation> GetSuballocation(size_t size) override;

		static uint32_t GetPadding();
		static size_t PadSize(size_t initialSize);

		virtual void SetData(void* inputData, size_t size) override;
		virtual void SetData(void* inputData, size_t offset, size_t size) override;
		virtual void SetLayout(const BufferLayout& layout) override { m_Specification.Layout = layout; };

		virtual inline BufferSpecification& GetSpecification() override { return m_Specification; }

		inline const AllocatedBuffer& GetAllocation() const { return m_Allocation; }
		inline AllocatedBuffer& GetAllocation() { return m_Allocation; }

		inline VkDescriptorBufferInfo GetBufferInfo(size_t offset = 0, size_t range = 0) { return CreateDescriptorInfo(offset, range); }

		static AllocatedBuffer CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, QueueFamilyOwnership ownership = QueueFamilyOwnership::QFO_UNDEFINED, std::string debugName = std::string());

		virtual const std::string& GetDebugName() const override { return m_Specification.DebugName; }
		

		virtual bool operator==(const Buffer& other) const override { return m_Handle == ((VulkanBuffer&)other).m_Handle; }

	private:
		void UploadToGPU();

		VkDescriptorBufferInfo CreateDescriptorInfo(size_t offset = 0, size_t range = 0);

	private:
		BufferHandle m_Handle;
		BufferSpecification m_Specification;

		AllocatedBuffer m_Allocation{};
		VkDescriptorBufferInfo m_DescriptorInfo{};

		AllocatedBuffer m_Staging{};
		bool m_StagingMapped = false;
		void* m_Data = nullptr;
		size_t m_Size = 0;
		size_t m_SuballocationOffset = 0;
				

		QueueFamilyOwnership m_Ownership = QueueFamilyOwnership::QFO_UNDEFINED;
	};

}
