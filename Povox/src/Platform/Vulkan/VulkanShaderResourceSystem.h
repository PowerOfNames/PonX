#pragma once
#include "Povox/Systems/ShaderResourceSystem.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDebug.h"
#include "Platform/Vulkan/VulkanDevice.h"

namespace Povox {

	struct AllocatedBuffer;
	class VulkanDescriptorLayoutCache;
	class VulkanShaderResourceSystem : public ShaderResourceSystem
	{
	public:
		VulkanShaderResourceSystem();
		virtual ~VulkanShaderResourceSystem();

		virtual bool Init() override;
		virtual void OnUpdate() override;
		virtual void Shutdown() override;

		

	private:
		Ref<VulkanDevice> m_Device = nullptr;

		Ref<VulkanDescriptorLayoutCache> m_DescriptorLayoutCache = nullptr;



		std::unordered_map<std::string, std::vector<AllocatedBuffer>> m_UniformBuffers;
		std::unordered_map<std::string, std::vector<AllocatedBuffer>> m_ShaderStorageBuffers;

		//std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};
}