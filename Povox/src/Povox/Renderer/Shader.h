#pragma once
#include "Povox/Renderer/RendererUID.h"

#include "Povox/Utils/ShaderResources.h"

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

namespace Povox {

	
	

	class Shader
	{
	public:
		virtual ~Shader() = default;
		virtual void Free() = 0;

		virtual const std::unordered_map<std::string, Ref<ShaderResourceDescription>>& GetResourceDescriptions() const = 0;

		virtual uint64_t GetRendererID() const = 0;
		virtual const std::string& GetDebugName() const = 0;

		virtual bool operator==(const Shader& other) const = 0;
		
		static Ref<Shader> Create(const std::string& filepath);		
	};

	class ShaderLibrary
	{
	public:
		ShaderLibrary();
		~ShaderLibrary() = default;

		void Shutdown();

		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);

		Ref<Shader> Load(const std::string& name, std::string& filepath);
		Ref<Shader> Load(const std::string& filepath);
		
		Ref<Shader> Get(const std::string& name);

		bool Contains(const std::string& name) const;

	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};
}
