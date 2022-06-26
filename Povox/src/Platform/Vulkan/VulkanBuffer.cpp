#include "pxpch.h"
#include "VulkanBuffer.h"
#include "VulkanDebug.h"
#include "VulkanCommands.h"

#include "VulkanContext.h"

namespace Povox {

//General VulkanBuffer stuff
	AllocatedBuffer VulkanBuffer::Create(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
	{
		VkBufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.pNext = nullptr;

		vertexBufferInfo.size = allocSize;
		vertexBufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaAllocInfo{};
		vmaAllocInfo.usage = memUsage;

		AllocatedBuffer newBuffer;

		PX_CORE_VK_ASSERT(vmaCreateBuffer(VulkanContext::GetAllocator(), &vertexBufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, nullptr), VK_SUCCESS, "Failed to create Buffer!");

		//TODO: add vmaDestroyBuffer to the deletion queue

		return newBuffer;
	}


// --------------------- VertexBuffer ----------------------------
	void VulkanVertexBuffer::Create(UploadContext& uploadContext, Mesh& mesh)
	{
		const size_t bufferSize = mesh.Vertices.size() * sizeof(VertexData);
		AllocatedBuffer stagingBuffer = VulkanBuffer::Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		VmaAllocator allocator = VulkanContext::GetAllocator();
		void* data;
		vmaMapMemory(allocator, stagingBuffer.Allocation, &data);
		memcpy(data, mesh.Vertices.data(), bufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);

		mesh.VertexBuffer = VulkanBuffer::Create(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanCommands::ImmidiateSubmitTrsf(uploadContext, [=](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = bufferSize;

				vkCmdCopyBuffer(cmd, stagingBuffer.Buffer, mesh.VertexBuffer.Buffer, 1, &copyRegion);
			});

		vmaDestroyBuffer(allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}

// --------------------- IndexBuffer ------------------------------
	void VulkanIndexBuffer::Create(UploadContext& uploadContext, Mesh& mesh)
	{
		const size_t bufferSize = mesh.Indices.size() * sizeof(mesh.Indices[0]);
		AllocatedBuffer stagingBuffer = VulkanBuffer::Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		VmaAllocator allocator = VulkanContext::GetAllocator();

		void* data;
		vmaMapMemory(allocator, stagingBuffer.Allocation, &data);
		memcpy(data, mesh.Indices.data(), bufferSize);
		vmaUnmapMemory(allocator, stagingBuffer.Allocation);

		mesh.IndexBuffer = VulkanBuffer::Create(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		VulkanCommands::ImmidiateSubmitTrsf(uploadContext, [=](VkCommandBuffer cmd)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = bufferSize;

				vkCmdCopyBuffer(cmd, stagingBuffer.Buffer, mesh.IndexBuffer.Buffer, 1, &copyRegion);
			});

		vmaDestroyBuffer(allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);
	}
}
