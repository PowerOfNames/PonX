#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Shader.h"

#include "Platform/Vulkan/VulkanShader.h"

#include "Povox/Core/Application.h"
#include "Povox/Core/Core.h"
#include "Povox/Core/Log.h"

namespace Povox {

	Ref<Shader> Shader::Create(const std::string& sources, const std::string& debugName)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanShader>(sources, debugName);
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
	Ref<Shader> Shader::Create(const std::filesystem::path& filepath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanShader>(filepath);
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
