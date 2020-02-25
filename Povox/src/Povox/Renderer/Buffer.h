#pragma once
#include "Povox/Renderer/Shader.h"

namespace Povox {

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexID;

		std::string ToString() const
		{
			std::stringstream ss;
			ss << "Vertex: Pos(" << Position.x << "|" << Position.y << "|" << Position.z
				<< "), Color(" << Color.r << "|" << Color.g << "|" << Color.g << "|" << Color.a
				<< "), Texture Crd(" << TexCoord.x << "|" << TexCoord.y << "), Texture ID(" << TexID << ")";
			return ss.str();
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const Vertex& v)
	{
		return os << v.ToString();
	}

	struct BufferElement
	{
		ShaderDataType Type;
		std::string Name;
		uint32_t Size;
		uint32_t Offset;
		bool Normalized;

		BufferElement() = default;

		BufferElement(ShaderDataType type, std::string name, bool normalized = false)
			: Type(type), Name(name), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

		uint32_t GetComponentCount() const
		{
			switch (Type)
			{
			case Povox::ShaderDataType::Float:	return 1;
			case Povox::ShaderDataType::Float2:	return 2;
			case Povox::ShaderDataType::Float3:	return 3;
			case Povox::ShaderDataType::Float4:	return 4;
			case Povox::ShaderDataType::Mat3:	return 3 * 3;	
			case Povox::ShaderDataType::Mat4:	return 4 * 4;
			case Povox::ShaderDataType::Int:	return 1;
			case Povox::ShaderDataType::Int2:	return 2;
			case Povox::ShaderDataType::Int3:	return 3;
			case Povox::ShaderDataType::Int4:	return 4;
			case Povox::ShaderDataType::Bool:	return 1;
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

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() =  default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Submit(Vertex* vertices, size_t size) const = 0;
		virtual void Submit(const std::vector<Vertex*>& vertices) const = 0;

		virtual void SetLayout(const BufferLayout& layout) = 0;
		virtual const BufferLayout& GetLayout() const = 0;

		virtual uint32_t GetID() const = 0;

		static Ref<VertexBuffer> Create(float* vertices, uint32_t size);
		static Ref<VertexBuffer> Create(Vertex* vertices, uint32_t size);
		static Ref<VertexBuffer> CreateBatch(uint32_t size);
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual uint32_t GetCount() const = 0;

		static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t size);
	};
}
