#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Shader.h"

namespace Povox {

	using MaterialHandle = UUID;

	class Material
	{
	public:
		virtual ~Material() = default;


		virtual bool Validate() = 0;


		virtual Ref<Shader> GetShader() = 0;
		virtual const Ref<Shader> GetShader() const = 0;


		virtual const MaterialHandle GetID() const = 0;
		static Ref<Material> Create(Ref<Shader> shader, const std::string& name = "MaterialName");
		
		virtual bool operator==(const Material& other) const = 0;
	};


	class MaterialSystem
	{
	public:
		virtual ~MaterialSystem() = default;

		virtual void Init() = 0;
		virtual void OnUpdate() = 0;
		virtual void Shutdown() = 0;
		
		virtual void Add(Ref<Material> material) = 0;

		virtual const Ref<Material>& GetMaterial(const std::string& name) const = 0;
		//virtual Ref<Material>& GetMaterial(const std::string& name) = 0;

		virtual const std::unordered_map<std::string, Ref<Material>>& GetMaterials() const = 0;
		virtual std::unordered_map<std::string, Ref<Material>>& GetMaterials() = 0;
	};

}
