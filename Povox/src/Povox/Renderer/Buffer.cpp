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
		case RendererAPI::API::NONE:
		{
			PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
			return nullptr;
		}
		case RendererAPI::API::Vulkan:
		{
			return CreateRef<VulkanBuffer>(specs);
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
