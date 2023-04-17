#include "pxpch.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {



	VulkanMaterial::VulkanMaterial(Ref<Shader> shader, const std::string& name)
		: m_Shader(shader), m_Name(name)	
	{

	}

	uint32_t VulkanMaterial::BindTexture(const std::string& name)
	{
		return Renderer::GetTextureSystem()->BindTexture(name);
	}

}
