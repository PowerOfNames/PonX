#pragma once
#include "Povox/Renderer/RendererUID.h"

#include <unordered_map>
#include <string>

#include <glm/glm.hpp>

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

	enum class ShaderResourceType
	{
		NONE = 0,
		SAMPLER,
		COMBINED_IMAGE_SAMPLER,
		SAMPLED_IMAGE,
		STORAGE_IMAGE,
		UNIFORM_TEXEL_BUFFER,
		STORAGE_TEXEL_BUFFER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		UNIFORM_BUFFER_DYNAMIC,
		STORAGE_BUFFER_DYNAMIC,
		INPUT_ATTACHMENT,
		PUSH_CONSTANT
	};
	namespace ToStringUtility {
		static std::string ShaderResourceTypeToString(ShaderResourceType type)
		{
			switch (type)
			{
				case ShaderResourceType::NONE: return "ShaderResourceType::NONE";
				case ShaderResourceType::SAMPLER: return "ShaderResourceType::SAMPLER";
				case ShaderResourceType::COMBINED_IMAGE_SAMPLER: return "ShaderResourceType::COMBINED_IMAGE_SAMPLER";
				case ShaderResourceType::SAMPLED_IMAGE: return "ShaderResourceType::SAMPLED_IMAGE";
				case ShaderResourceType::STORAGE_IMAGE: return "ShaderResourceType::STORAGE_IMAGE";
				case ShaderResourceType::UNIFORM_TEXEL_BUFFER: return "ShaderResourceType::UNIFORM_TEXEL_BUFFER";
				case ShaderResourceType::STORAGE_TEXEL_BUFFER: return "ShaderResourceType::STORAGE_TEXEL_BUFFER";
				case ShaderResourceType::UNIFORM_BUFFER: return "ShaderResourceType::UNIFORM_BUFFER";
				case ShaderResourceType::STORAGE_BUFFER: return "ShaderResourceType::STORAGE_BUFFER";
				case ShaderResourceType::UNIFORM_BUFFER_DYNAMIC: return "ShaderResourceType::UNIFORM_BUFFER_DYNAMIC";
				case ShaderResourceType::STORAGE_BUFFER_DYNAMIC: return "ShaderResourceType::STORAGE_BUFFER_DYNAMIC";
				case ShaderResourceType::INPUT_ATTACHMENT: return "ShaderResourceType::INPUT_ATTACHMENT";
				case ShaderResourceType::PUSH_CONSTANT: return "ShaderResourceType::PUSH_CONSTANT";
				default: 
				{
					PX_CORE_WARN("ShaderResourceType not defined!");
					return "Missing ShaderResourceType!";
				}				
			}
		}
	}

	enum class ShaderStage : std::uint32_t
	{
		NONE					= 0,
		VERTEX					= 1 << 0,
		TESSELLATION_CONTROL	= 1 << 1,
		TESSELLATION_EVALUATION = 1 << 2,
		GEOMETRY				= 1 << 3,
		FRAGMENT				= 1 << 4,
		COMPUTE					= 1 << 5
	};
	inline ShaderStage operator|(ShaderStage stage1, ShaderStage stage2)
	{
		return static_cast<ShaderStage>(
			static_cast<std::underlying_type_t<ShaderStage>>(stage1) |
			static_cast<std::underlying_type_t<ShaderStage>>(stage2)
			);
	}
	inline ShaderStage operator|=(ShaderStage stage1, ShaderStage stage2)
	{
		return static_cast<ShaderStage>(
			static_cast<std::underlying_type_t<ShaderStage>>(stage1) |
			static_cast<std::underlying_type_t<ShaderStage>>(stage2)
			);
	}
	inline ShaderStage operator&(ShaderStage stage1, ShaderStage stage2)
	{
		return static_cast<ShaderStage>(
			static_cast<std::underlying_type_t<ShaderStage>>(stage1) &
			static_cast<std::underlying_type_t<ShaderStage>>(stage2)
			);
	}
	
	namespace ToStringUtility
	{
		static std::string ShaderStageToString(ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::NONE: return "ShaderStage::NONE";
				case ShaderStage::COMPUTE: return "ShaderStage::COMPUTE";
				case ShaderStage::VERTEX: return "ShaderStage::VERTEX";
				case ShaderStage::GEOMETRY: return "ShaderStage::GEOMETRY";
				case ShaderStage::TESSELLATION_CONTROL: return "ShaderStage::TESSELLATION_CONTROL";
				case ShaderStage::TESSELLATION_EVALUATION: return "ShaderStage::TESSELLATION_EVALUATION";
				case ShaderStage::FRAGMENT: return "ShaderStage::FRAGMENT";
				default:
				{
					PX_CORE_WARN("ShaderStage not defined!");
					return "Missing ShaderStage!";
				}

			}
		}
	}

	// TODO: Clean up this mess
	struct ShaderResourceDescription
	{
		std::string Name;
		uint32_t Set;
		uint32_t Binding;
		ShaderStage Stage;

		ShaderResourceType ResourceType = ShaderResourceType::NONE;

		ShaderStage Stages;
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;
		virtual void Free() = 0;

		virtual const std::unordered_map<std::string, ShaderResourceDescription>& GetResourceDescriptions() const = 0;

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
