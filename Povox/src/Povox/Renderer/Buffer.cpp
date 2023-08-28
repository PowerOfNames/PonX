#include "pxpch.h"
#include "Povox/Renderer/Buffer.h"
#include "Platform/Vulkan/VulkanBuffer.h"

#include "Povox/Renderer/RendererAPI.h"
#include "Povox/Renderer/Renderer.h"

namespace Povox {

	Ref<Buffer> Buffer::Create(const BufferSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanBuffer>(specs);
			}
			case RendererAPI::API::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	

	// ShaderResources
	UniformBuffer::UniformBuffer(const BufferLayout& layout, bool perFrame /*= true*/)
		: m_Layout(layout), m_PerFrame(perFrame)
	{
		CreateBuffers();
	}

	void UniformBuffer::SetData(void* data, size_t size)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_Buffers[frameIndex]->SetData(data, size);
	}

	void UniformBuffer::Set(void* data, const std::string& name, size_t size)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		const BufferElement* element = m_Layout.GetElement(name);
		uint32_t offset = element->Offset;

		m_Buffers[frameIndex]->SetData(data, offset, size);
	}

	void UniformBuffer::CreateBuffers()
	{
		BufferSpecification specs{};
		specs.Usage = BufferUsage::UNIFORM_BUFFER;
		specs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Layout = m_Layout;
		specs.ElementSize = m_Layout.GetStride();
		specs.Size = m_Layout.GetStride();

		if (!m_PerFrame)
		{
			m_Buffers.push_back(Buffer::Create(specs));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Buffers.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Buffers.size(); i++)
		{
			m_Buffers[i] = Buffer::Create(specs);
		}
	}

	Ref<Buffer> UniformBuffer::GetBuffer(uint32_t frameIndex /*= 0*/)
	{
		PX_CORE_ASSERT(frameIndex < m_Buffers.size(), "FrameIndex is too large!");
		return m_Buffers[frameIndex];
	}


	StorageBuffer::StorageBuffer(const BufferLayout& layout, size_t count, bool perFrame /*= true*/)
		: m_Layout(layout), m_PerFrame(perFrame), m_ElementCount(count)
	{
		CreateBuffers();
	}

	void StorageBuffer::SetData(void* data, size_t size)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_Buffers[frameIndex]->SetData(data, size);
	}

	void StorageBuffer::SetData(void* data, uint32_t index, size_t size)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_Buffers[frameIndex]->SetData(data, index, size);
	}

	void StorageBuffer::Set(void* data, uint32_t index, const std::string& name, size_t size)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		const BufferElement* element = m_Layout.GetElement(name);
		uint32_t offset = index * element->Size + element->Offset;

		m_Buffers[frameIndex]->SetData(data, offset, size);
	}

	void StorageBuffer::CreateBuffers()
	{
		BufferSpecification specs{};
		specs.Usage = BufferUsage::STORAGE_BUFFER;
		specs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Layout = m_Layout;
		specs.ElementCount = m_ElementCount;
		specs.ElementSize = m_Layout.GetStride();
		specs.Size = specs.ElementSize * m_ElementCount;

		if (!m_PerFrame)
		{
			m_Buffers.push_back(Buffer::Create(specs));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Buffers.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Buffers.size(); i++)
		{
			m_Buffers[i] = Buffer::Create(specs);
		}
	}

	Ref<Buffer> StorageBuffer::GetBuffer(uint32_t frameIndex /*= 0*/)
	{
		PX_CORE_ASSERT(frameIndex < m_Buffers.size(), "FrameIndex is too large!");
		return m_Buffers[frameIndex];
	}

	

}
