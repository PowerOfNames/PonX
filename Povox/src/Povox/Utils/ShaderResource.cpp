#include "pxpch.h"
#include "Povox/Utils/ShaderResource.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {

	StorageImage::StorageImage(ImageSpecification& spec, bool perFrame /*= true*/)
		: ShaderResource(ShaderResourceType::STORAGE_IMAGE, perFrame, spec.DebugName)
	{
		m_Format = spec.Format;
		m_Usages = spec.Usages;
		m_Width = spec.Width;
		m_Height = spec.Height;
		m_PerFrame = perFrame;

		if (!m_PerFrame)
		{
			spec.DebugName = m_Name + "_Single";
			m_Images.push_back(Image2D::Create(spec));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Images.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			spec.DebugName = m_Name + "_Frame" + std::to_string(i);
			m_Images[i] = Image2D::Create(spec);
		}
	}

	void StorageImage::SetData(void* data, size_t size)
	{

	}

	void StorageImage::SetData(void* data, uint32_t index, size_t size)
	{

	}

	void StorageImage::Set(void* data, uint32_t index, const std::string& name, size_t size)
	{

	}

	Ref<Image2D> StorageImage::GetImage(uint32_t frameIndex /*= 0*/)
	{
		PX_CORE_ASSERT(m_Images.size(), "Images are unset!");

		if (!m_PerFrame)
			return m_Images[0];

		PX_CORE_ASSERT(frameIndex < m_Images.size(), "FrameIndex is too large!");
		return m_Images[frameIndex];
	}

	UniformBuffer::UniformBuffer(const BufferLayout& layout, const std::string& name /*= "UniformBufferDefault"*/,bool perFrame /*= true*/)
		: m_Layout(layout), ShaderResource(ShaderResourceType::UNIFORM_BUFFER, perFrame, name)
	{
		BufferSpecification specs{};
		specs.Usage = BufferUsage::UNIFORM_BUFFER;
		specs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Layout = m_Layout;
		specs.ElementSize = m_Layout.GetStride();
		specs.Size = m_Layout.GetStride();

		if (!m_PerFrame)
		{
			specs.DebugName = m_Name + "_Single";
			m_Buffers.push_back(Buffer::Create(specs));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Buffers.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Buffers.size(); i++)
		{
			specs.DebugName = m_Name + "_Frame" + std::to_string(i);
			m_Buffers[i] = Buffer::Create(specs);
		}
	}

	void UniformBuffer::SetData(void* data, size_t size)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			m_Buffers[0]->SetData(data, size);
		else
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
			m_Buffers[frameIndex]->SetData(data, size);
		}
	}

	void UniformBuffer::Set(void* data, const std::string& name, size_t size)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			m_Buffers[0]->SetData(data, size);
		else
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			const BufferElement* element = m_Layout.GetElement(name);
			uint32_t offset = element->Offset;

			m_Buffers[frameIndex]->SetData(data, offset, size);
		}
	}

	Ref<Buffer> UniformBuffer::GetBuffer(uint32_t frameIndex /*= 0*/)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			return m_Buffers[0];

		PX_CORE_ASSERT(frameIndex < m_Buffers.size(), "FrameIndex is too large!");
		return m_Buffers[frameIndex];
	}


	StorageBuffer::StorageBuffer(const BufferLayout& layout, size_t count, const std::string& name /*= "StorageBufferDefault"*/, bool perFrame /*= true*/)
		: m_Layout(layout), m_ElementCount(count), ShaderResource(ShaderResourceType::STORAGE_BUFFER, perFrame, name)
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
			specs.DebugName = m_Name + "_Single";
			m_Buffers.push_back(Buffer::Create(specs));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Buffers.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Buffers.size(); i++)
		{
			specs.DebugName = m_Name + "_Frame" + std::to_string(i);
			m_Buffers[i] = Buffer::Create(specs);
		}
	}

	void StorageBuffer::SetData(void* data, size_t size)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			m_Buffers[0]->SetData(data, size);
		else
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			m_Buffers[frameIndex]->SetData(data, size);
		}
	}

	void StorageBuffer::SetData(void* data, uint32_t index, size_t size)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			m_Buffers[0]->SetData(data, index, size);
		else
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			m_Buffers[frameIndex]->SetData(data, index, size);
		}

	}

	void StorageBuffer::Set(void* data, uint32_t index, const std::string& name, size_t size)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			m_Buffers[0]->SetData(data, index, size);
		else
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			const BufferElement* element = m_Layout.GetElement(name);
			uint32_t offset = index * element->Size + element->Offset;

			m_Buffers[frameIndex]->SetData(data, offset, size);
		}
	}

	Ref<Buffer> StorageBuffer::GetBuffer(uint32_t frameIndex /*= 0*/)
	{
		PX_CORE_ASSERT(m_Buffers.size(), "Buffers are unset!");

		if (!m_PerFrame)
			return m_Buffers[0];

		PX_CORE_ASSERT(frameIndex < m_Buffers.size(), "FrameIndex is too large!");
		return m_Buffers[frameIndex];
	}

	ShaderResource::ShaderResource(ShaderResourceType resourceType/* = ShaderResourceType::NONE*/, bool perFrame, const std::string& name)
		: m_ResourceType(resourceType), m_PerFrame(perFrame), m_Name(name) {}

}