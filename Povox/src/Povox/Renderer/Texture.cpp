#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Texture.h"

#include "Platform/Vulkan/VulkanImage2D.h"

namespace Povox {

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, uint32_t channels)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::Vulkan:
		{
			return CreateRef<VulkanTexture2D>(width, height, channels);
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

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanTexture2D>(path);
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
