#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Image2D.h"

namespace Povox {

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
		ACCELERATION_STRUCTURE,
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
				case ShaderResourceType::ACCELERATION_STRUCTURE: return "ShaderResourceType::ACCELERATION_STRUCTURE";
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
		NONE = 0,
		VERTEX = 1,
		TESSELLATION_CONTROL = 1 << 1,
		TESSELLATION_EVALUATION = 1 << 2,
		GEOMETRY = 1 << 3,
		FRAGMENT = 1 << 4,
		COMPUTE = 1 << 5
	};
	inline ShaderStage operator|(ShaderStage stage1, ShaderStage stage2)
	{
		return static_cast<ShaderStage>(
			static_cast<std::underlying_type_t<ShaderStage>>(stage1) |
			static_cast<std::underlying_type_t<ShaderStage>>(stage2)
			);
	}
	inline ShaderStage& operator|=(ShaderStage& stage1, ShaderStage stage2)
	{
		stage1 = stage1 | stage2;
		return stage1;
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
			std::string output = "Stages";

			if ((stage & ShaderStage::VERTEX) == ShaderStage::VERTEX)
				output += "::VERTEX";
			if ((stage & ShaderStage::TESSELLATION_CONTROL) == ShaderStage::TESSELLATION_CONTROL)
				output += "::TESSELLATION_CONTROL";
			if ((stage & ShaderStage::TESSELLATION_EVALUATION) == ShaderStage::TESSELLATION_EVALUATION)
				output += "::TESSELLATION_EVALUATION";
			if ((stage & ShaderStage::GEOMETRY) == ShaderStage::GEOMETRY)
				output += "::GEOMETRY";
			if ((stage & ShaderStage::FRAGMENT) == ShaderStage::FRAGMENT)
				output += "::FRAGMENT";
			if ((stage & ShaderStage::COMPUTE) == ShaderStage::COMPUTE)
				output += "::COMPUTE";

			if (output == "Stages")
				return "Unknown Stage!";

			return output;
		}
	}

	// TODO: Clean up this mess
	struct ShaderResourceDescription
	{
		std::string Name;
		ShaderResourceType ResourceType = ShaderResourceType::NONE;

		uint32_t Set;
		uint32_t Binding;
		uint32_t Count;
		ShaderStage Stages;
	};


	struct ShaderResourceSpecification
	{
		std::string Name = "ShaderResource";
		bool PerFrame = true;
		ShaderResourceType ResourceType = ShaderResourceType::NONE;

		BufferSpecification* BufferSpec = nullptr;
		ImageSpecification* ImageSpec = nullptr;
		//SamplerSpecification SamplerSpec{};
	};

	class ShaderResource
	{
	public:
		ShaderResource(bool perFrame, const std::string& debugName);
		virtual ~ShaderResource() = default;

	protected:
		std::string m_DebugName;
		bool m_PerFrame;
	};

	class StorageImage : public ShaderResource
	{
	public:
		StorageImage(ImageSpecification& spec, bool perFrame = true);
		virtual ~StorageImage() = default;

		void SetData(void* data, size_t size);
		void SetData(void* data, uint32_t index, size_t size);
		void Set(void* data, uint32_t index, const std::string& name, size_t size);


		Ref<Image2D> GetImage(uint32_t frameIndex = 0);

	private:
		ImageFormat m_Format{};
		ImageUsages m_Usages;

		uint32_t m_Width;
		uint32_t m_Height;
		std::vector<Ref<Image2D>> m_Images;
	};

	class UniformBuffer : public ShaderResource
	{
	public:
		UniformBuffer(const BufferLayout& layout, const std::string& name = "UniformBufferDefault", bool perFrame = true);
		~UniformBuffer() = default;

		void SetData(void* data, size_t size);
		void Set(void* data, const std::string& name, size_t size);


		Ref<Buffer> GetBuffer(uint32_t frameIndex = 0);

	private:
		BufferLayout m_Layout;

		std::vector<Ref<Buffer>> m_Buffers;
	};

	class StorageBuffer : public ShaderResource
	{
	public:
		StorageBuffer(const BufferLayout& layout, size_t count, const std::string& name = "StorageBufferDefault", bool perFrame = true);
		~StorageBuffer() = default;

		void SetData(void* data, size_t size);
		void SetData(void* data, uint32_t index, size_t size);
		void Set(void* data, uint32_t index, const std::string& name, size_t size);


		Ref<Buffer> GetBuffer(uint32_t frameIndex = 0);

	private:
		BufferLayout m_Layout;

		size_t m_ElementCount;
		std::vector<Ref<Buffer>> m_Buffers;
	};
}

