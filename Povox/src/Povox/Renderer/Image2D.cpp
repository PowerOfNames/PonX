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
			
		}
		PX_CORE_ASSERT(true, "Unknown Renderer::API!");
	}

}
