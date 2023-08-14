#include "pxpch.h"
#include "Platform/Vulkan/VulkanMaterialSystem.h"


namespace Povox {



	VulkanMaterial::VulkanMaterial(Ref<Shader> shader, const std::string& name)
		: m_Shader(shader), m_Name(name)	
	{

	}

	VulkanMaterial::~VulkanMaterial()
	{

	}

	bool VulkanMaterial::Validate()
	{
		return false;
	}


	VulkanMaterialSystem::VulkanMaterialSystem()
	{

	}


	VulkanMaterialSystem::~VulkanMaterialSystem()
	{

	}

	void VulkanMaterialSystem::Init()
	{

	}


	void VulkanMaterialSystem::OnUpdate()
	{

	}


	void VulkanMaterialSystem::Shutdown()
	{

	}


	void VulkanMaterialSystem::Add(Ref<Material> material)
	{

	}


	const Povox::Ref<Povox::Material>& VulkanMaterialSystem::GetMaterial(const std::string& name) const
	{
		if (m_Materials.find(name) == m_Materials.end())
		{
			PX_CORE_WARN("Material '{}' not registered in MaterialSystem! Returning default material.");
			// TODO: Return DefaultMaterial -> check with layouts
			return nullptr;
		}
		return m_Materials.at(name);
	}


// 	Povox::Ref<Povox::Material>& VulkanMaterialSystem::GetMaterial(const std::string& name)
// 	{
// 		if (m_Materials.find(name) == m_Materials.end())
// 		{
// 			PX_CORE_WARN("Material '{}' not registered in MaterialSystem! Returning default material.");
// 			// TODO: Return DefaultMaterial -> check with layouts
// 			return nullptr;
// 		}
// 		return m_Materials.at(name);
// 	}

}
