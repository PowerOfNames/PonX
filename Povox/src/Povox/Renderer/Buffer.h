#pragma once
#include "Povox/Core/BitField.h"
#include "Povox/Core/UUID.h"

#include "Povox/Renderer/Utilities.h"

#include "Povox/Renderer/Shader.h"

namespace Povox {

	using BufferHandle = UUID;

	struct BufferElement
	{
		ShaderDataType Usage;
		std::string Name;
		uint32_t Size;
		uint32_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(ShaderDataType type, std::string name, bool normalized = false)
			: Usage(type), Name(name), Size(ShaderUtility::ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

		uint32_t GetComponentCount() const
		{
			switch (Usage)
			{
			case ShaderDataType::Float:		return 1;
			case ShaderDataType::Float2:	return 2;
			case ShaderDataType::Float3:	return 3;
			case ShaderDataType::Float4:	return 4;
			case ShaderDataType::Mat3:		return 3 * 3;
			case ShaderDataType::Mat4:		return 4 * 4;
			case ShaderDataType::Int:		return 1;
			case ShaderDataType::Int2:		return 2;
			case ShaderDataType::Int3:		return 3;
			case ShaderDataType::Int4:		return 4;
			case ShaderDataType::Bool:		return 1;
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
		inline const BufferElement* GetElement(const std::string& name) { return FindElement(name); };

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
		const BufferElement* FindElement(const std::string& name)
		{
			for (auto& element : m_BufferElements)
			{
				if (element.Name == name)
					return &element;
			}
			PX_CORE_ASSERT(true, "Element not set in Layout!");
			return nullptr;
		}

	private:
		std::vector<BufferElement> m_BufferElements;
		uint32_t m_Stride = 0;
	};

	enum class BufferUsage
	{
		UNDEFINED = 0,

		VERTEX_BUFFER = 1 << 1,
		INDEX_BUFFER_32 = 1 << 2,
		INDEX_BUFFER_64 = 1 << 3,
		UNIFORM_BUFFER = 1 << 4,
		UNIFORM_BUFFER_DYNAMIC = 1 << 5,
		STORAGE_BUFFER = 1 << 6,
		STORAGE_BUFFER_DYNAMIC = 1 << 7,

		DATA_STATIC = 1 << 8,
		DATA_DYNAMIC = 1 << 9
	};
	template<>
	struct enable_bitmask_operators<BufferUsage>
	{
		static constexpr bool enable = true;
	};

	namespace EnumToString {
		static std::string BufferUsageString(BufferUsage usage)
		{
			std::string output = "Buffer Usages: ";

			if ((usage & BufferUsage::UNDEFINED) == BufferUsage::UNDEFINED)
				output += "::UNDEFINED";
			if ((usage & BufferUsage::VERTEX_BUFFER) == BufferUsage::VERTEX_BUFFER)
				output += "::VERTEX_BUFFER";
			if ((usage & BufferUsage::INDEX_BUFFER_32) == BufferUsage::INDEX_BUFFER_32)
				output += "::INDEX_BUFFER_32";
			if ((usage & BufferUsage::INDEX_BUFFER_64) == BufferUsage::INDEX_BUFFER_64)
				output += "::INDEX_BUFFER_64";
			if ((usage & BufferUsage::UNIFORM_BUFFER) == BufferUsage::UNIFORM_BUFFER)
				output += "::UNIFORM_BUFFER";
			if ((usage & BufferUsage::UNIFORM_BUFFER_DYNAMIC) == BufferUsage::UNIFORM_BUFFER_DYNAMIC)
				output += "::UNIFORM_BUFFER_DYNAMIC";
			if ((usage & BufferUsage::STORAGE_BUFFER) == BufferUsage::STORAGE_BUFFER)
				output += "::STORAGE_BUFFER";
			if ((usage & BufferUsage::STORAGE_BUFFER_DYNAMIC) == BufferUsage::STORAGE_BUFFER_DYNAMIC)
				output += "::STORAGE_BUFFER_DYNAMIC";
			if ((usage & BufferUsage::DATA_STATIC) == BufferUsage::DATA_STATIC)
				output += "::DATA_STATIC";
			if ((usage & BufferUsage::DATA_DYNAMIC) == BufferUsage::DATA_DYNAMIC)
				output += "::DATA_DYNAMIC";

			if (output == "Buffer Usages: ")
				return "Unknown Usage!";

			return output;
		}
	}
	

	struct BufferSpecification
	{
		BufferUsage Usage = BufferUsage::UNDEFINED;
		MemoryUtils::MemoryUsage MemUsage = MemoryUtils::MemoryUsage::UNDEFINED;

		BufferLayout Layout;

		uint32_t ElementCount = 0;
		uint32_t ElementSize = 0;
		size_t Size = 0;

		std::string DebugName = "Buffer";
	};
	
	struct BufferSuballocation;
	class Buffer : public enable_shared_from_base<Buffer>
	{
	public:
		virtual ~Buffer() = default;
		virtual void Free() = 0;

		virtual void SetData(void* data, size_t size) = 0;
		virtual void SetData(void* inputData, size_t offset, size_t size) = 0;

		virtual void SetLayout(const BufferLayout& layout) = 0;
		virtual BufferSpecification& GetSpecification() = 0;

		virtual Ref<BufferSuballocation> GetSuballocation(size_t size) = 0;
		
		static uint32_t GetPadding() { return 0; }
		static size_t PadSize(size_t initialSize) { return initialSize; }

		static Ref<Buffer> Create(const BufferSpecification& specs);
		

		virtual BufferHandle GetID() const = 0;
		virtual const std::string& GetDebugName() const = 0;

		virtual bool operator==(const Buffer& other) const = 0;
	};

	// TODO: Refactor into class
	struct BufferSuballocation
	{
		BufferSuballocation() = default;
		BufferSuballocation(Ref<Buffer> buffer, size_t offset, size_t range) : Buffer(buffer), Offset(offset), Range(range) {};

		void SetData(void* data, size_t partialOffset, size_t size) { Buffer->SetData(data, Offset + partialOffset, size); };

		Ref<Buffer> Buffer = nullptr;
		size_t Offset = 0;
		size_t Range = 0;
	};
}
