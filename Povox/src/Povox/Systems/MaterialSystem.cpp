#include "pxpch.h"
#include "Povox/Systems/MaterialSystem.h"
#include "Platform/Vulkan/VulkanMaterialSystem.h"

#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/RendererAPI.h"

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
