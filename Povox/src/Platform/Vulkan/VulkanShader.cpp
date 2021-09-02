#include "pxpch.h"
#include "VulkanShader.h"
#include "VulkanDebug.h"

#include <cstdint>
#include <fstream>

namespace Povox {

	//-------------------VulkanShader---------------------------
	VkShaderModule VulkanShader::Create(VkDevice logicalDevice, const std::string& filepath)
	{
		std::ifstream stream(filepath, std::ios::ate | std::ios::binary);
		bool isOpen = stream.is_open();
		PX_CORE_ASSERT(isOpen, "Failed to open file!");

		size_t fileSize = (size_t)stream.tellg();
		std::vector<char> buffer(fileSize);

		stream.seekg(0);
		stream.read(buffer.data(), fileSize);

		PX_CORE_INFO("Shadercode size: '{0}'", buffer.size());

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = buffer.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

		VkShaderModule shaderModule;
		PX_CORE_VK_ASSERT(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule), VK_SUCCESS, "Failed to create shader module!");
		return shaderModule;
	}

//-------------------VulkanDescriptorPool---------------------------
	void VulkanDescriptorPool::CreatePool(VkDevice logicalDevice, int imageCount)
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(imageCount);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(imageCount);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(imageCount);


		PX_CORE_VK_ASSERT(vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &m_Pool), VK_SUCCESS, "Failed to create descriptor pool!");
	}

	void VulkanDescriptorPool::DestroyPool(VkDevice logicalDevice)
	{
		vkDestroyDescriptorPool(logicalDevice, m_Pool, nullptr);
	}

	void VulkanDescriptorPool::SetLayout(VkDevice logicalDevice)
	{
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.binding = 0;		// binding in the shader
		uboBinding.descriptorCount = 1;		// set could be an array -> count is number of elements
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.pImmutableSamplers = nullptr;	// optional ->  for image sampling

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;		// binding in the shader
		samplerLayoutBinding.descriptorCount = 1;		// set could be an array -> count is number of elements
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;	// optional ->  for image sampling

		std::array< VkDescriptorSetLayoutBinding, 2> bindings{ uboBinding , samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();


		PX_CORE_VK_ASSERT(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &m_Layout), VK_SUCCESS, "Failed to create descriptor set layout!");
	}

	void VulkanDescriptorPool::DestroyLayout(VkDevice logicalDevice)
	{
		vkDestroyDescriptorSetLayout(logicalDevice, m_Layout, nullptr);
	}

	void VulkanDescriptorPool::CreateSets(VkDevice logicalDevice, int imageCount, const std::vector<VkBuffer>& uniformBuffers, VkImageView textureImageView, VkSampler textureSampler)
	{
		std::vector<VkDescriptorSetLayout> layouts(imageCount, m_Layout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_Pool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
		allocInfo.pSetLayouts = layouts.data();

		m_Sets.resize(imageCount);
		VkResult result = vkAllocateDescriptorSets(logicalDevice, &allocInfo, m_Sets.data());
		PX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor sets!");

		for (size_t i = 0; i < imageCount; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_Sets[i];
			descriptorWrites[0].dstBinding = 0;		// again, binding in the shader
			descriptorWrites[0].dstArrayElement = 0;		// descriptors can be arrays
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;		// how many elements in the array
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pImageInfo = nullptr;	// optional image data

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_Sets[i];
			descriptorWrites[1].dstBinding = 1;		// again, binding in the shader
			descriptorWrites[1].dstArrayElement = 0;		// descriptors can be arrays
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;		// how many elements in the array
			descriptorWrites[1].pBufferInfo = &bufferInfo;
			descriptorWrites[1].pImageInfo = &imageInfo;	// optional image data

			vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
}