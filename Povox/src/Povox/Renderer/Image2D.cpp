#include "pxpch.h"
#include "Image2D.h"

#include "Platform/Vulkan/VulkanImage2D.h"
#include "Povox/Core/Core.h"
#include "Povox/Core/Log.h"

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

	Ref<Image2D> Image2D::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanImage2D>(width, height);
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

}
