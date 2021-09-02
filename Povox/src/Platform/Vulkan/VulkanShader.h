#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace Povox {

	struct UniformBufferObject
	{
		alignas(16) glm::mat4 ModelMatrix;
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
	};

	class VulkanShader
	{
	public:
		static VkShaderModule Create(VkDevice logicalDevice, const std::string& filepath);
	};

	//ToDO define the struct to hold possible attributes to give to the setLayout function inside the renderer
	struct VulkanDescriptor
	{

	};

	class VulkanDescriptorPool
	{
	public:
		VulkanDescriptorPool() = default;
		~VulkanDescriptorPool() = default;
		void CreatePool(VkDevice logicalDevice, int imageCount);
		void DestroyPool(VkDevice logicalDevice);

		void SetLayout(VkDevice logicalDevice);
		VkDescriptorSetLayout GetLayout() const { return m_Layout; }
		void DestroyLayout(VkDevice logicalDevice);

		void CreateSets(VkDevice logicalDevice, int imageCount, const std::vector<VkBuffer>& uniformBuffers, VkImageView textureImageView, VkSampler textureSampler);
		const std::vector<VkDescriptorSet>& GetSets() const { return m_Sets; }
	private:
		VkDescriptorPool m_Pool;
		VkDescriptorSetLayout m_Layout;
		std::vector<VkDescriptorSet> m_Sets;
	};
}