#include "pxpch.h"
#include "Povox/Utils/ShaderResource.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {

	ShaderResource::ShaderResource(ShaderResourceType resourceType /*= ShaderResourceType::NONE*/, bool perFrame /*= true*/, const std::string& name /*= "ShaderResource"*/)
		: m_ResourceType(resourceType), m_PerFrame(perFrame), m_Name(name)
	{
	}


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
			spec.DebugName = m_Name + "_Frame_" + std::to_string(i);
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

	UniformBuffer::UniformBuffer(const BufferLayout& layout, const std::string& name /*= "UniformBufferDefault"*/, bool perFrame /*= true*/)
		: m_Layout(layout), ShaderResource(ShaderResourceType::UNIFORM_BUFFER, perFrame, name)
	{
		BufferSpecification specs{};
		specs.Usage = BufferUsage::UNIFORM_BUFFER;
		specs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Layout = m_Layout;
		specs.ElementSize = m_Layout.GetStride();
		specs.Size = Buffer::PadSize(m_Layout.GetStride());

		if (!m_PerFrame)
		{
			specs.DebugName = name + "_Single";
			m_Buffers.push_back(Buffer::Create(specs));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Buffers.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Buffers.size(); i++)
		{
			specs.DebugName = name + "_Frame_" + std::to_string(i);
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

//---- Storage Buffer ----
	StorageBuffer::StorageBuffer(const BufferLayout& layout, size_t elements, const std::string& name /*= "StorageBufferDefault"*/, bool perFrame /*= true*/)
		: m_Layout(layout), ShaderResource(ShaderResourceType::STORAGE_BUFFER, perFrame, name)
	{
		BufferSpecification specs{};
		specs.Usage = BufferUsage::STORAGE_BUFFER;
		specs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Layout = m_Layout;
		specs.ElementCount = elements;
		specs.ElementSize = m_Layout.GetStride();
		specs.Size = specs.ElementSize * elements;

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
			specs.DebugName = m_Name + "_Frame_" + std::to_string(i);
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


//---- StorageBuffer Dynamic ----
	StorageBufferDynamic::StorageBufferDynamic(const BufferLayout& layout, size_t totalSize /*= 1*/, const std::string& name /*= "StorageBufferDynamicDefault"*/, bool perFrame /*= true*/)
		: m_Layout(layout), ShaderResource(ShaderResourceType::STORAGE_BUFFER_DYNAMIC, perFrame, name)
	{
		BufferSpecification specs{};
		specs.Usage = BufferUsage::STORAGE_BUFFER;
		specs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Layout = layout;
		specs.ElementCount = 0;
		specs.ElementSize = layout.GetStride();
		specs.Size = totalSize;
		specs.DebugName = m_Name + "_Single";
				
		m_Buffer = Buffer::Create(specs);
	}

	void StorageBufferDynamic::AddDescriptor(const std::string& name, size_t size, FrameBehaviour usage /*= FrameBehaviour::STANDARD*/, uint8_t initialFrame /*= 0*/, const std::string& linkedDescriptorName /*= NULL*/)
	{	
		Ref<BufferSuballocation> sub = m_Buffer->GetSuballocation(size);

		if (sub == nullptr)
		{
			PX_CORE_ERROR("Failed to create a suballocation!");
			return;
		}
		if (m_ContainedDescriptors.find(name) != m_ContainedDescriptors.end())
		{
			PX_CORE_WARN("Descriptor already defined in {}", m_Name);
		}

		m_ContainedDescriptors[name] = DynamicBufferElement{ sub, initialFrame, usage, linkedDescriptorName };
	}

	const std::vector<uint32_t>& StorageBufferDynamic::GetOffsets(const std::string& name, uint32_t currentFrameIndex) const
	{
		std::vector<uint32_t> offsets;

// 		switch (m_Usage)
// 		{
// 			case FrameBehaviour::STANDARD:
// 			{
// 				break;
// 			}
// 			case FrameBehaviour::FRAME_ITERATE:
// 			{
// 
// 				break;
// 			}
// 			case FrameBehaviour::FRAME_SWAP_IN_OUT:
// 			{
// 
// 				break;
// 			}
// 		}
		offsets.push_back(1);

		return offsets;
	}	

	const uint32_t StorageBufferDynamic::GetOffset(const std::string& name, uint32_t currentFrameIndex) const
	{
		if (m_ContainedDescriptors.find(name) == m_ContainedDescriptors.end())
		{
			PX_CORE_ERROR("Descriptor {} not contained! Returning 0", name);
			return 0;
		}

		const DynamicBufferElement& element = m_ContainedDescriptors.at(name);
		switch (element.Behaviour)
		{
			case FrameBehaviour::STANDARD:
			{
				return element.Suballocation->Offset;
			}
			case FrameBehaviour::FRAME_ITERATE:
			{

				break;
			}
			case FrameBehaviour::FRAME_SWAP_IN_OUT:
			{
				const std::string& link = element.LinkedDescriptorName;

				if (currentFrameIndex == 0)
					return element.Suballocation->Offset;
				else
				{
					if (m_ContainedDescriptors.find(link) == m_ContainedDescriptors.end())
					{
						PX_CORE_ERROR("Descriptor link {} not contained! Returning original offset of {}", link, name);
						return element.Suballocation->Offset;
					}
					return m_ContainedDescriptors.at(link).Suballocation->Offset;
				}
			}
		}
	}

	StorageBufferDynamic::DynamicBufferElement StorageBufferDynamic::GetDescriptorInfo(const std::string& name) 
	{
		if (m_ContainedDescriptors.find(name) == m_ContainedDescriptors.end())
		{
			PX_CORE_ERROR("StorageBufferDynamic::GetDescriptorInfo: Descriptor {} not contained!", name);
			return DynamicBufferElement();
		}

		return m_ContainedDescriptors.at(name);
	}

	void StorageBufferDynamic::SetDescriptorData(const std::string& name, void* data, size_t size, size_t partialOffset /*= 0*/)
	{
		if (m_ContainedDescriptors.find(name) == m_ContainedDescriptors.end())
		{
			PX_CORE_ERROR("StorageBufferDynamic::SetDescriptorData: Descriptor {} not contained!", name);
			return;
		}
		m_ContainedDescriptors.at(name).Suballocation->SetData(data, partialOffset, size);
	}

}