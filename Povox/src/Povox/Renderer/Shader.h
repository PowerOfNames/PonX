#pragma once

#include <unordered_map>
#include <string>

namespace Povox {

	enum class ShaderDataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
	};

	static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case Povox::ShaderDataType::Float:	return 4;	
		case Povox::ShaderDataType::Float2:	return 4 * 2;
		case Povox::ShaderDataType::Float3:	return 4 * 3;
		case Povox::ShaderDataType::Float4:	return 4 * 4;
		case Povox::ShaderDataType::Mat3:	return 4 * 3 * 3;
		case Povox::ShaderDataType::Mat4:	return 4 * 4 * 4;
		case Povox::ShaderDataType::Int:	return 4;	
		case Povox::ShaderDataType::Int2:	return 4 * 2;
		case Povox::ShaderDataType::Int3:	return 4 * 3;
		case Povox::ShaderDataType::Int4:	return 4 * 4;
		case Povox::ShaderDataType::Bool:	return 1;	
		}

		PX_CORE_ASSERT(false, "No such ShaderDataType defined!");
		return 0;
	}

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual	void Unbind() const = 0;

		virtual const std::string& GetName() const = 0;

		static Ref<Shader> Create(const std::string& filepath);
		static Ref<Shader> Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	};

	class ShaderLibrary
	{
	public:

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
