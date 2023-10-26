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

		virtual uint64_t GetRendererID() const override { return m_RID; }

		virtual void SetData(void* inputData, size_t size) override;
		virtual void SetData(void* inputData, uint32_t offset, size_t size) override;
		virtual void SetLayout(const BufferLayout& layout) override { m_Specification.Layout = layout; };

		virtual inline BufferSpecification& GetSpecification() override { return m_Specification; }

		inline const AllocatedBuffer& GetAllocation() const { return m_Allocation; }
		inline AllocatedBuffer& GetAllocation() { return m_Allocation; }

		inline VkDescriptorBufferInfo& GetBufferInfo() { return m_DescriptorInfo; }

		static AllocatedBuffer CreateAllocation(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, QueueFamilyOwnership ownership = QueueFamilyOwnership::QFO_UNDEFINED, std::string debugName = std::string());

		virtual const std::string& GetDebugName() const override { return m_Specification.DebugName; }

		virtual bool operator==(const Buffer& other) const override { return m_RID == ((VulkanBuffer&)other).m_RID; }

	private:
		void UploadToGPU();

		VkDescriptorBufferInfo CreateDescriptorInfo();

	private:
		RendererUID m_RID;
		BufferSpecification m_Specification;

		AllocatedBuffer m_Allocation{};
		VkDescriptorBufferInfo m_DescriptorInfo{};

		AllocatedBuffer m_Staging{};
		bool m_StagingMapped = false;
		void* m_Data = nullptr;
		size_t m_Size = 0;
				

		QueueFamilyOwnership m_Ownership = QueueFamilyOwnership::QFO_UNDEFINED;
	};
}
