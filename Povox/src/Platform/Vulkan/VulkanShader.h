#pragma once
#include "Povox/Renderer/Shader.h"


#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Povox {

	struct GPUBufferObject
	{
		glm::mat4 ModelMatrix;
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

	struct VertexInputDescription
	{
		std::vector<VkVertexInputBindingDescription> Bindings;
		std::vector<VkVertexInputAttributeDescription> Attributes;

		VkPipelineVertexInputStateCreateFlags Flags = 0;
	};

	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const std::string& filepath);
		virtual ~VulkanShader() = default;

		//static VkShaderModule Create(VkDevice logicalDevice, const std::string& filepath);


		virtual const std::string& GetName() const { return m_DebugName; }

		// Uniforms
		virtual void SetInt(const std::string& name, int value) {}
		virtual void SetIntArray(const std::string & name, int* values, uint32_t count) {}

		virtual void SetFloat(const std::string & name, float value) {}
		virtual void SetFloat2(const std::string & name, const glm::vec2 & vector) {}
		virtual void SetFloat3(const std::string & name, const glm::vec3 & vector) {}
		virtual void SetFloat4(const std::string & name, const glm::vec4 & vector) {}

		virtual void SetMat3(const std::string & name, const glm::mat3 & matrix) {}
		virtual void SetMat4(const std::string & name, const glm::mat4 & matrix) {}

		inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetBufferBindings() const { return m_DescriptorSetBufferBindings; }
		inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetImageBindings() const { return m_DescriptorSetImageBindings; }
		inline std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetBufferBindings() { return m_DescriptorSetBufferBindings; }
		inline std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetImageBindings() { return m_DescriptorSetImageBindings; }

		inline const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		inline std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return m_DescriptorSetLayouts; }

		inline VertexInputDescription& GetVertexInputDescription() { return m_VertexInputDescription; }
		inline const VertexInputDescription& GetVertexInputDescription() const { return m_VertexInputDescription; }


		VkShaderModule& GetModule(VkShaderStageFlagBits stage);

	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& sources);
		void CompileOrGetVulkanBinaries(std::unordered_map<VkShaderStageFlagBits, std::string> sources);
		void Reflect();

	private:
		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> m_SourceCodes;
		std::unordered_map<VkShaderStageFlagBits, VkShaderModule> m_Modules;

		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetBufferBindings;
		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetImageBindings;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

		VertexInputDescription m_VertexInputDescription{};

		const std::string m_FilePath = "";
		std::string m_DebugName = "Shader name";
	};
}
