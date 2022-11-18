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

	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const std::string& filepath);
		virtual ~VulkanShader() = default;

		//static VkShaderModule Create(VkDevice logicalDevice, const std::string& filepath);


		virtual const std::string& GetName() const { return "Shader name!"; }

		// Uniforms
		virtual void SetInt(const std::string& name, int value) {};
		virtual void SetIntArray(const std::string & name, int* values, uint32_t count) {}

		virtual void SetFloat(const std::string & name, float value) {}
		virtual void SetFloat2(const std::string & name, const glm::vec2 & vector) {}
		virtual void SetFloat3(const std::string & name, const glm::vec3 & vector) {}
		virtual void SetFloat4(const std::string & name, const glm::vec4 & vector) {}

		virtual void SetMat3(const std::string & name, const glm::mat3 & matrix) {}
		virtual void SetMat4(const std::string & name, const glm::mat4 & matrix) {}

		inline const VkShaderModule GetModule(VkShaderStageFlagBits stage) const;
		inline const ShaderInfo GetInfo() const { return m_Info; }

	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& sources);
		void CompileOrGetVulkanBinaries(std::unordered_map<VkShaderStageFlagBits, std::string> sources);
		//void Reflect();

	private:
		ShaderInfo m_Info;
		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> m_SourceCodes;
		std::unordered_map<VkShaderStageFlagBits, VkShaderModule> m_Modules;


		struct Descriptor
		{
			VkDescriptorSetLayout m_DescriptorLayout = VK_NULL_HANDLE;
			VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		};
		Descriptor m_Descriptor;				
	};
}
