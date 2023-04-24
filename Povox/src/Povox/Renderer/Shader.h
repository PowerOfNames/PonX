#pragma once
#include "Povox/Renderer/RendererUID.h"

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

namespace Povox {

	class Shader
	{
	public:
		virtual ~Shader() = default;
		virtual void Free() = 0;

		virtual uint64_t GetRendererID() const = 0;
		virtual const std::string& GetDebugName() const = 0;

		virtual bool operator==(const Shader& other) const = 0;
		
		static Ref<Shader> Create(const std::string& filepath);		
		
		// Uniforms
		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) = 0;

		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& vector) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& vector) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& vector) = 0;

		virtual void SetMat3(const std::string& name, const glm::mat3& matrix) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& matrix) = 0;
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
