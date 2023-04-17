#include "pxpch.h"
#include "Povox/Renderer/Material.h"

#include "Platform/Vulkan/VulkanMaterial.h"

#include "Povox/Renderer/RendererAPI.h"
#include "Povox/Renderer/Renderer.h"

namespace Povox {



	Ref<Material> Material::Create(Ref<Shader> shader, const std::string& name)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanMaterial>(shader, name);
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
