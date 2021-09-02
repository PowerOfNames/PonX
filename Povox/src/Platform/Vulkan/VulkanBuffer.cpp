#include "pxpch.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"
#include "VulkanDebug.h"

namespace Povox {

//General VulkanBuffer stuff
	void VulkanBuffer::CreateBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkBuffer& buffer, VkDeviceMemory& memory, uint32_t familyIndexCount, uint32_t* familyIndices, VkSharingMode sharingMode)
	{
		VkBufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.size = size;
		vertexBufferInfo.usage = usage;
		vertexBufferInfo.sharingMode = sharingMode;
		vertexBufferInfo.queueFamilyIndexCount = familyIndexCount;
		vertexBufferInfo.pQueueFamilyIndices = familyIndices;

		PX_CORE_VK_ASSERT(vkCreateBuffer(logicalDevice, &vertexBufferInfo, nullptr, &buffer), VK_SUCCESS, "Failed to create vertex buffer!");


		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &requirements);

		VkMemoryAllocateInfo memoryInfo{};
		memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryInfo.allocationSize = requirements.size;
		memoryInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(physicalDevice, requirements.memoryTypeBits, properties);

		PX_CORE_VK_ASSERT(vkAllocateMemory(logicalDevice, &memoryInfo, nullptr, &memory), VK_SUCCESS, "Failed to allocate vertex buffer memory!");

		vkBindBufferMemory(logicalDevice, buffer, memory, 0);
	}


// --------------------- VertexBuffer ----------------------------
	VulkanVertexBuffer::VulkanVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
	{
		/*QueueFamilyIndices indices = m_Device->FindQueueFamilies(physicalDevice, m_Surface);
		uint32_t queueFamilyIndices[] = {
			indices.GraphicsFamily.value(),
			indices.TransferFamily.value()
		};
		//uint32_t indiceSize = static_cast<uint32_t>(sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]));

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();
		VulkanBuffer::CreateBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		void* data;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);
		VulkanBuffer::CreateBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer,
			m_Memory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		//CopyBuffer(stagingBuffer, m_Buffer, bufferSize);
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);*/
	}

	void VulkanVertexBuffer::Destroy(VkDevice logicalDevice)
	{
		vkDestroyBuffer(logicalDevice, m_Buffer, nullptr);
	}

	void VulkanVertexBuffer::Bind() const
	{

	}

	void VulkanVertexBuffer::Unbind() const
	{

	}

	void VulkanVertexBuffer::SetData(const void* data, uint32_t size)
	{

	}

	const BufferLayout& VulkanVertexBuffer::GetLayout() const
	{
		BufferLayout bufferlayout;
		return bufferlayout;
	}
	void VulkanVertexBuffer::SetLayout(const BufferLayout& layout)
	{

	}

// --------------------- IndexBuffer ------------------------------
	VulkanIndexBuffer::VulkanIndexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
	{
		/*QueueFamilyIndices indices = m_Device->FindQueueFamilies(physicalDevice, m_Surface);
		uint32_t queueFamilyIndices[] = {
			indices.GraphicsFamily.value(),
			indices.TransferFamily.value()
		};
		uint32_t indiceSize = static_cast<uint32_t>(sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]));

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VkDeviceSize bufferSize = sizeof(uint16_t) * m_Indices.size();
		VulkanBuffer::CreateBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
			stagingBufferMemory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		void* data;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Indices.data(), (size_t)bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		VulkanBuffer::CreateBuffer(logicalDevice, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer,
			m_Memory, indiceSize, queueFamilyIndices, VK_SHARING_MODE_CONCURRENT);

		CopyBuffer(stagingBuffer, m_Buffer, bufferSize);

		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);*/
	}

	void VulkanIndexBuffer::Destroy(VkDevice logicalDevice)
	{
		vkDestroyBuffer(logicalDevice, m_Buffer, nullptr);
	}
}