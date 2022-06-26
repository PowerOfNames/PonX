#pragma once
#include "Povox/Renderer/Buffer.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"


#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Povox {

	struct VertexInputDescription
	{
		std::vector< VkVertexInputBindingDescription> Bindings;
		std::vector< VkVertexInputAttributeDescription> Attributes;

		VkPipelineVertexInputStateCreateFlags Flags = 0;
	};
	struct VertexData
	{
		glm::vec3 Position;
		glm::vec3 Color;
		glm::vec2 TexCoord;

		static VertexInputDescription GetVertexDescription()
		{
			VertexInputDescription output;

			VkVertexInputBindingDescription vertexBindingDescription{};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = sizeof(VertexData);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			output.Bindings.push_back(vertexBindingDescription);
		
			VkVertexInputAttributeDescription positionAttribute{};
			positionAttribute.binding = 0;
			positionAttribute.location = 0;
			positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			positionAttribute.offset = offsetof(VertexData, Position);

			VkVertexInputAttributeDescription colorAttribute{};
			colorAttribute.binding = 0;
			colorAttribute.location = 1;
			colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
			colorAttribute.offset = offsetof(VertexData, Color);
			
			VkVertexInputAttributeDescription uvAttributes{};
			uvAttributes.binding = 0;
			uvAttributes.location = 2;
			uvAttributes.format = VK_FORMAT_R32G32_SFLOAT;
			uvAttributes.offset = offsetof(VertexData, TexCoord);

			output.Attributes.push_back(positionAttribute);
			output.Attributes.push_back(colorAttribute);
			output.Attributes.push_back(uvAttributes);

			return output;
		}
	};

	struct AllocatedBuffer
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
	};

	struct Mesh
	{
		std::vector<VertexData> Vertices;
		AllocatedBuffer VertexBuffer;

		std::vector<uint16_t> Indices;
		AllocatedBuffer IndexBuffer;
	};

	class VulkanBuffer
	{
	public:
		static AllocatedBuffer Create(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);
	};

	class VulkanVertexBuffer
	{
	public:
		static void Create(UploadContext& uploadContext, Mesh& mesh);
	};

	class VulkanIndexBuffer
	{
	public:
		static void Create(UploadContext& uploadContext, Mesh& mesh);
	};

	class VulkanUniformBuffer
	{
	public:

	private:
		VkBuffer m_Buffer;

	};
}
