#include "pxpch.h"
#include "Povox/Core/Log.h"
#include "Povox/Core/Core.h"

#include "Image2D.h"
#include "Platform/Vulkan/VulkanImage2D.h"

#include "Povox/Renderer/Renderer.h"

namespace Povox {

	Ref<Image2D> Image2D::Create(const ImageSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::Vulkan:
		{
			return CreateRef<VulkanImage2D>(spec);
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

	Ref<Image2D> Image2D::Create(uint32_t width, uint32_t height, uint32_t channels)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanImage2D>(width, height, channels);
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

	StorageImage::StorageImage(ImageFormat format, uint32_t width, uint32_t height, const ImageUsages& usages /*= { ImageUsage::STORAGE }*/, const std::string& name /*= "StorageImageDefault"*/, bool shared /* = false*/, bool perFrame /*= true*/)
		: m_Format(format), m_Usages(usages), m_Width(width), m_Height(height), m_DebugName(name), m_Shared(shared), m_PerFrame(perFrame)
	{
		ImageSpecification specs{};
		specs.Usages = m_Usages;
		specs.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		specs.Format = m_Format;
		specs.DedicatedSampler = true;
		specs.Width = m_Width;
		specs.Height = m_Height;
		specs.ChannelCount = 4;

		if (!m_PerFrame)
		{
			specs.DebugName = m_DebugName + "_Single";
			m_Images.push_back(Image2D::Create(specs));
			return;
		}

		uint32_t framesPerFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_Images.resize(framesPerFlight);
		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			specs.DebugName = m_DebugName + "_Frame" + std::to_string(i);
			m_Images[i] = Image2D::Create(specs);
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

}
