#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace Povox {

	struct CameraUniformBuffer
	{
		alignas(16) glm::mat4 ViewMatrix;
		alignas(16) glm::mat4 ProjectionMatrix;
		alignas(16) glm::mat4 ViewProjMatrix;
	};

	struct SceneUniformBufferD
	{
		glm::vec4 FogColor;
		glm::vec4 FogDistance;
		glm::vec4 AmbientColor;
		glm::vec4 SunlightDirection;
		glm::vec4 SunlightColor;
	};

	struct PushConstants
	{
		alignas(16) glm::mat4 ModelMatrix;
	};

	class VulkanShader
	{
	public:
		static VkShaderModule Create(VkDevice logicalDevice, const std::string& filepath);

	private:
		static std::vector<uint32_t> CompileOrGetVulkanBinaries(const std::string& source);
	};

	class VulkanDescriptor
	{
	public:
		static VkDescriptorPool CreatePool(VkDevice logicalDevice);
		static VkDescriptorSetLayout CreateLayout(VkDevice logicalDevice, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);
		static VkDescriptorSet CreateSet(VkDevice logicalDevice, VkDescriptorPool pool, VkDescriptorSetLayout* layout);
	};
}
