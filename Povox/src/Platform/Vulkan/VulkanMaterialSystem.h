#pragma once
#include "Povox/Systems/MaterialSystem.h"

#include "Povox/Core/Log.h"
#include "Povox/Core/Core.h"

#include "Povox/Renderer/RendererUID.h"
#include "Povox/Renderer/Shader.h"


namespace Povox {



	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(Ref<Shader> shader, const std::string& name);
		virtual ~VulkanMaterial();


		virtual bool Validate() override;

		virtual Ref<Shader> GetShader() override { return m_Shader; }
		virtual const Ref<Shader> GetShader() const override { return m_Shader; }

		virtual const uint64_t GetRendererID() const override { return m_RUID; }
		virtual bool operator==(const Material& other) const override { return m_RUID == ((VulkanMaterial&)other).m_RUID; }
	private:
		RendererUID m_RUID;
		Ref<Shader> m_Shader = nullptr;
		std::string m_Name;

		// We have textures, ssbos, ubos, push_constants
		// textures just need an image
		// ssbos and ubos get a layout
		// if an object contains a material with an ssbo, we combine their values together in the MaterialSystem
		// push_constants need to be queried pr oject during the rendering
		
		// what happens with dynamic buffers?
	};



	class VulkanMaterialSystem : public MaterialSystem
	{
	public:
		VulkanMaterialSystem();
		virtual ~VulkanMaterialSystem();

		virtual void Init() override;
		virtual void OnUpdate() override;
		virtual void Shutdown() override;

		virtual void Add(Ref<Material> material) override;

		virtual const Ref<Material>& GetMaterial(const std::string& name) const override;
		//virtual Ref<Material>& GetMaterial(const std::string& name) override;

		virtual const std::unordered_map<std::string, Ref<Material>>& GetMaterials() const override { return m_Materials; };
		virtual std::unordered_map<std::string, Ref<Material>>& GetMaterials() override { return m_Materials; };

	private:
		std::unordered_map<std::string, Ref<Material>> m_Materials;
		std::unordered_map<std::string, std::vector<Ref<ShaderResourceDescription>>> m_ShaderResources;
	};

}
