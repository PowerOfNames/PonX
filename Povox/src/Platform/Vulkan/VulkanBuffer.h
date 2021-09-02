#pragma once
#include "Povox/Renderer/Buffer.h"

#include <vulkan/vulkan.h>

namespace Povox {

	struct VertexData
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TexCoord;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription vertexBindingDescription{};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = sizeof(VertexData);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return vertexBindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions{};
			vertexAttributeDescriptions[0].binding = 0;
			vertexAttributeDescriptions[0].location = 0;
			vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexAttributeDescriptions[0].offset = offsetof(VertexData, Position);

			vertexAttributeDescriptions[1].binding = 0;
			vertexAttributeDescriptions[1].location = 1;
			vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexAttributeDescriptions[1].offset = offsetof(VertexData, Color);

			vertexAttributeDescriptions[2].binding = 0;
			vertexAttributeDescriptions[2].location = 2;
			vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributeDescriptions[2].offset = offsetof(VertexData, TexCoord);

			return vertexAttributeDescriptions;
		}
	};

	class VulkanBuffer
	{
	public:

		static void CreateBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
			VkBuffer& buffer, VkDeviceMemory& memory, uint32_t familyIndexCount = 0, uint32_t* familyIndices = nullptr, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	};

	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);
		virtual ~VulkanVertexBuffer() = default;

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void SetData(const void* data, uint32_t size) override;

		virtual const BufferLayout& GetLayout() const override;
		virtual void SetLayout(const BufferLayout& layout) override;

		void Destroy(VkDevice logicalDevice);

		VkBuffer Get() const { return m_Buffer; }
		VkDeviceMemory GetMemory() const { return m_Memory; }
	private:
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;
		BufferLayout m_Layout;

		const std::vector<VertexData> m_Vertices = {
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

			{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
		};
	};

	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);
		virtual ~VulkanIndexBuffer() = default;

		void Destroy(VkDevice logicalDevice);

		VkBuffer Get() const { return m_Buffer; }
		VkDeviceMemory GetMemory() const { return m_Memory; }
		const std::vector<uint16_t>& GetIndices() const { return m_Indices; }
	private:
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;
		uint32_t m_Count;

		const std::vector<uint16_t> m_Indices = {
			 0, 1, 2, 2, 3, 0,
			 4, 5, 6, 6, 7, 4
		};
	};

	class VulkanUniformBuffer
	{
	public:

	private:
		VkBuffer m_Buffer;

	};
}
