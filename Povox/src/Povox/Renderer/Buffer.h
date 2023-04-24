#pragma once
#include "Povox/Core/Log.h"
#include "Povox/Renderer/RendererUID.h"
#include "Povox/Renderer/Utilities.h"

namespace Povox {

	struct BufferElement
	{
		ShaderUtils::ShaderDataType Usage;
		std::string Name;
		uint32_t Size;
		uint32_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(ShaderUtils::ShaderDataType type, std::string name, bool normalized = false)
			: Usage(type), Name(name), Size(ShaderUtils::ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

		uint32_t GetComponentCount() const
		{
			switch (Usage)
			{
			case ShaderUtils::ShaderDataType::Float:	return 1;
			case ShaderUtils::ShaderDataType::Float2:	return 2;
			case ShaderUtils::ShaderDataType::Float3:	return 3;
			case ShaderUtils::ShaderDataType::Float4:	return 4;
			case ShaderUtils::ShaderDataType::Mat3:		return 3 * 3;	
			case ShaderUtils::ShaderDataType::Mat4:		return 4 * 4;
			case ShaderUtils::ShaderDataType::Int:		return 1;
			case ShaderUtils::ShaderDataType::Int2:		return 2;
			case ShaderUtils::ShaderDataType::Int3:		return 3;
			case ShaderUtils::ShaderDataType::Int4:		return 4;
			case ShaderUtils::ShaderDataType::Bool:		return 1;
			}

			PX_CORE_ASSERT(false, "No such ShaderDataType defined!");
			return 0;
		}
	};

	class BufferLayout
	{
	public:
		BufferLayout() {}

		BufferLayout(const std::initializer_list<BufferElement>& bufferElements)
			: m_BufferElements(bufferElements)
		{
			CalculateOffsetAndStride();
		}

		inline const std::vector<BufferElement>& GetElements() const { return m_BufferElements; }
		inline const uint32_t GetStride() const { return m_Stride; }

		std::vector<BufferElement>::iterator begin() { return m_BufferElements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_BufferElements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_BufferElements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_BufferElements.end(); }
	private:
		void CalculateOffsetAndStride()
		{
			m_Stride = 0;
			for (auto& element : m_BufferElements)
			{
				element.Offset = m_Stride;
				m_Stride += element.Size;
			}
		}
	private:
		std::vector<BufferElement> m_BufferElements;
		uint32_t m_Stride = 0;
	};

	enum class BufferUsage
	{
		UNDEFINED = 0,

		VERTEX_BUFFER = 1,
		INDEX_BUFFER = 2,
		UNIFORM_BUFFER = 3,
		STORAGE_BUFFER = 4
	};
	namespace EnumToString {
		constexpr const char* BufferUsageString(BufferUsage usage)
		{
			switch (usage)
			{
				case BufferUsage::UNDEFINED: return "UNDEFINED";
				case BufferUsage::VERTEX_BUFFER: return "VERTEX_BUFFER";
				case BufferUsage::INDEX_BUFFER: return "INDEX_BUFFER";
				case BufferUsage::UNIFORM_BUFFER: return "UNIFORM_BUFFER";
				case BufferUsage::STORAGE_BUFFER: return "STORAGE_BUFFER";
				default: return "Missing BufferUsageString conversion!";
			}
		}
	}
	

	struct BufferSpecification
	{
		BufferUsage Usage = BufferUsage::UNDEFINED;
		MemoryUtils::MemoryUsage MemUsage = MemoryUtils::MemoryUsage::UNDEFINED;

		BufferLayout VertexLayout;

		uint32_t ElementCount = 0;
		uint32_t ElementOffset = 0;
		uint32_t ElementSize = 0;

		size_t Size = ElementCount * ElementSize;
		void* Data;

		std::string DebugName = "Buffer";
	};

	class Buffer
	{
	public:
		virtual ~Buffer() = default;
		virtual void Free() = 0;

		virtual void SetData(void* data, size_t size) = 0;
		//virtual void SetLayout(const BufferLayout& vertexLayout) = 0;
		virtual BufferSpecification& GetSpecification() = 0;

		virtual uint64_t GetRendererID() const = 0;

		static Ref<Buffer> Create(const BufferSpecification& specs);

		virtual const std::string& GetDebugName() const = 0;

		virtual bool operator==(const Buffer& other) const = 0;
	};
}
