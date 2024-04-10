#pragma once
#include "Povox/Renderer/Shader.h"

#include "Platform/Vulkan/VulkanUtilities.h"

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

	class Pipeline;
	class ComputePipeline;
	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const std::string& sources, const std::string& debugName = "Shader");
		VulkanShader(const std::filesystem::path& filepath);
		virtual ~VulkanShader() = default;

		virtual void Free() override;
		virtual bool Recompile(const std::string& sources) override;

		virtual const std::unordered_map<std::string, Ref<ShaderResourceDescription>>& GetResourceDescriptions() const override { return m_ShaderResourceDescriptions; }
		inline const std::map<uint32_t, VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		inline std::map<uint32_t, VkDescriptorSetLayout>& GetDescriptorSetLayouts() { return m_DescriptorSetLayouts; }
		inline VertexInputDescription& GetVertexInputDescription() { return m_VertexInputDescription; }
		inline const VertexInputDescription& GetVertexInputDescription() const { return m_VertexInputDescription; }
		inline const std::unordered_map<VkShaderStageFlagBits, VkShaderModule>& VulkanShader::GetModules() { return m_Modules; }

		virtual inline void AddPipeline(Ref<Pipeline> pipeline) override { m_Pipeline = pipeline; };
		virtual inline void AddComputePipeline(Ref<ComputePipeline> pipeline) override { m_ComputePipeline = pipeline; };

		virtual void Rename(const std::string& newName) override { m_DebugName = newName; }

		virtual const std::string& GetDebugName() const { return m_DebugName; }
		virtual UUID GetID() const override { return m_Handle; }

		virtual bool operator==(const Shader& other) const override { return m_Handle == ((VulkanShader&)other).m_Handle; }

	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& sources);
		bool CompileOrGetVulkanBinaries(std::unordered_map<VkShaderStageFlagBits, std::string> sources, bool forceRecompile = false);

		bool CheckDescriptorHash(VkShaderStageFlagBits stage, uint32_t set, const DescriptorLayoutInfo& layoutInfo);
		
		void SortLayoutInfoBindings(DescriptorLayoutInfo& layoutInfo);

		bool Reflect();
		bool ReflectVertexStage(const std::vector<uint32_t>* moduleData, bool printDebug = false);


	private:
		ShaderHandle m_Handle;
		const std::filesystem::path m_SPVPath = "";
		std::string m_DebugName;
		Ref<Pipeline> m_Pipeline = nullptr;
		Ref<ComputePipeline> m_ComputePipeline = nullptr;
		std::unordered_map<VkShaderStageFlagBits, std::map<uint32_t, size_t>> m_LayoutInfoHashes;

		std::unordered_map<std::string, Ref<ShaderResourceDescription>> m_ShaderResourceDescriptions;
		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> m_SourceCodes;
		std::unordered_map<VkShaderStageFlagBits, VkShaderModule> m_Modules;
		std::map<uint32_t, VkDescriptorSetLayout> m_DescriptorSetLayouts;
		VertexInputDescription m_VertexInputDescription{};
	};
}
