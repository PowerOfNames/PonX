#pragma once
#include "Povox/Core/UUID.h"
//#include "Povox/Renderer/Pipeline.h"

#include <unordered_map>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>

#include "FileWatch.h"


namespace Povox {

	enum class ShaderDataType
	{
		None = 0,
		Float,
		Float2,
		Float3,
		Float4,
		Mat3,
		Mat4,
		Int,
		Int2,
		Int3,
		Int4,
		UInt,
		Long,
		ULong,
		Bool
	};
	namespace ToStringUtility {
		static std::string ShaderDataTypeToString(ShaderDataType type)
		{
			switch (type)
			{
				case ShaderDataType::None: return "ShaderDataType::NONE";
				case ShaderDataType::Float: return "ShaderDataType::Float";
				case ShaderDataType::Float2: return "ShaderDataType::Float2";
				case ShaderDataType::Float3: return "ShaderDataType::Float3";
				case ShaderDataType::Float4: return "ShaderDataType::Float4";
				case ShaderDataType::Mat3: return "ShaderDataType::Mat3";
				case ShaderDataType::Mat4: return "ShaderDataType::Mat4";
				case ShaderDataType::Int: return "ShaderDataType::Int";
				case ShaderDataType::Int2: return "ShaderDataType::Int2";
				case ShaderDataType::Int3: return "ShaderDataType::Int3";
				case ShaderDataType::Int4: return "ShaderDataType::Int4";
				case ShaderDataType::UInt: return "ShaderDataType::UInt";
				case ShaderDataType::Long: return "ShaderDataType::Long";
				case ShaderDataType::ULong: return "ShaderDataType::ULong";
				case ShaderDataType::Bool: return "ShaderDataType::Bool";
				default:
				{
					PX_CORE_WARN("ShaderDataType not defined!");
					return "Missing ShaderDataType!";
				}

			}
		}
	}
	namespace ShaderUtility {

		static uint32_t ShaderDataTypeSize(ShaderDataType type)
		{
			switch (type)
			{
				case ShaderDataType::Float:		return 4;
				case ShaderDataType::Int:		return 4;
				case ShaderDataType::UInt:		return 4;
				case ShaderDataType::Float2:	return 4 * 2;
				case ShaderDataType::Float3:	return 4 * 3;
				case ShaderDataType::Float4:	return 4 * 4;
				case ShaderDataType::Mat3:		return 4 * 3 * 3;
				case ShaderDataType::Mat4:		return 4 * 4 * 4;
				case ShaderDataType::Int2:		return 4 * 2;
				case ShaderDataType::Int3:		return 4 * 3;
				case ShaderDataType::Int4:		return 4 * 4;
				case ShaderDataType::Long:		return 8;
				case ShaderDataType::ULong:		return 8;
				case ShaderDataType::Bool:		return 1;
			}

			PX_CORE_ASSERT(false, "No such ShaderDataType defined!");
			return 0;
		}
	}
	
	using ShaderHandle = UUID;

	struct ShaderResourceDescription;
	class Pipeline;
	class ComputePipeline;
	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Free() = 0;
		virtual bool Recompile(const std::string& sources) = 0;

		virtual const std::unordered_map<std::string, Ref<ShaderResourceDescription>>& GetResourceDescriptions() const = 0;

		virtual void AddPipeline(Ref<Pipeline> pipeline) = 0;
		virtual void AddComputePipeline(Ref<ComputePipeline> pipeline) = 0;

		virtual void Rename(const std::string& newName) = 0;
		virtual const std::string& GetDebugName() const = 0;
		virtual UUID GetID() const = 0;

		virtual bool operator==(const Shader& other) const = 0;
		
		static Ref<Shader> Create(const std::string& sources, const std::string& debugName);
		static Ref<Shader> Create(const std::filesystem::path& filepath);
	};
	
}
