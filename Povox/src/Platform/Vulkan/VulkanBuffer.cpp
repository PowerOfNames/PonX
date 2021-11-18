#include "pxpch.h"
#include "VulkanBuffer.h"
#include "VulkanDebug.h"
#include "VulkanCommands.h"

namespace Povox {

//General VulkanBuffer stuff
	AllocatedBuffer VulkanBuffer::Create(const VulkanCoreObjects& core, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
	{
		VkBufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;

		vertexBufferInfo.size = allocSize;
		vertexBufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = memUsage;

		AllocatedBuffer newBuffer;

		PX_CORE_VK_ASSERT(vmaCreateBuffer(core.Allocator, &vertexBufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, nullptr), VK_SUCCESS, "Failed to create Buffer!");

		//TODO: add vmaDestroyBuffer to the deletion queue

		return newBuffer;
	}


// --------------------- VertexBuffer ----------------------------
	void VulkanVertexBuffer::Create(const VulkanCoreObjects& core, UploadContext& uploadContext, Mesh& mesh)
	{
		const size_t bufferSize = mesh.Vertices.size() * sizeof(VertexData);
		AllocatedBuffer stagingBuffer = VulkanBuffer::Create(core, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(core.Allocator, stagingBuffer.Allocation, &data);
		memcpy(data, mesh.Vertices.data(), bufferSize);
		vmaUnmapMemory(core.Allocator, stagingBuffer.Allocation);

		mesh.VertexBuffer = VulkanBuffer::Create(core, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		VulkanCommands::CopyBuffer(core, uploadContext, stagingBuffer.Buffer, mesh.VertexBuffer.Buffer, bufferSize);

		vmaDestroyBuffer(core.Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

// --------------------- IndexBuffer ------------------------------
	void VulkanIndexBuffer::Create(const VulkanCoreObjects& core, UploadContext& uploadContext, Mesh& mesh)
	{
		const size_t bufferSize = mesh.Indices.size() * sizeof(mesh.Indices[0]);
		AllocatedBuffer stagingBuffer = VulkanBuffer::Create(core, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(core.Allocator, stagingBuffer.Allocation, &data);
		memcpy(data, mesh.Indices.data(), bufferSize);
		vmaUnmapMemory(core.Allocator, stagingBuffer.Allocation);

		mesh.IndexBuffer = VulkanBuffer::Create(core, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanCommands::CopyBuffer(core, uploadContext, stagingBuffer.Buffer, mesh.IndexBuffer.Buffer, bufferSize);

		vmaDestroyBuffer(core.Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}
}
