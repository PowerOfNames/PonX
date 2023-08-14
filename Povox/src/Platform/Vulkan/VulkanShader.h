#pragma once
#include "Povox/Renderer/Shader.h"


#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Povox {

	struct GPUBufferObject
	{
		glm::mat4 ModelMatrix;
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
		virtual void Free() override;

		virtual const std::unordered_map<std::string, ShaderResourceDescription>& GetResourceDescriptions() const override { return m_ShaderResourceDescriptions; }

		virtual uint64_t GetRendererID() const override { return m_RID; }

		inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetBufferBindings() const { return m_DescriptorSetBufferBindings; }
		inline const std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetImageBindings() const { return m_DescriptorSetImageBindings; }
		inline std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetBufferBindings() { return m_DescriptorSetBufferBindings; }
		inline std::vector<VkDescriptorSetLayoutBinding>& GetDescriptorSetImageBindings() { return m_DescriptorSetImageBindings; }

		inline const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		inline std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return m_DescriptorSetLayouts; }

		inline VertexInputDescription& GetVertexInputDescription() { return m_VertexInputDescription; }
		inline const VertexInputDescription& GetVertexInputDescription() const { return m_VertexInputDescription; }


		inline const std::unordered_map<VkShaderStageFlagBits, VkShaderModule>& VulkanShader::GetModules() { return m_Modules; }

		virtual const std::string& GetDebugName() const { return m_DebugName; }

		virtual bool operator==(const Shader& other) const override { return m_RID == ((VulkanShader&)other).m_RID; }
	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& sources);
		void CompileOrGetVulkanBinaries(std::unordered_map<VkShaderStageFlagBits, std::string> sources);
		void Reflect();

	private:
		RendererUID m_RID;

		std::unordered_map<std::string, ShaderResourceDescription> m_ShaderResourceDescriptions;

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> m_SourceCodes;
		std::unordered_map<VkShaderStageFlagBits, VkShaderModule> m_Modules;

		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetBufferBindings;
		std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetImageBindings;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

		VertexInputDescription m_VertexInputDescription{};

		const std::string m_FilePath = "";
		std::string m_DebugName = "Shader";
	};
}
