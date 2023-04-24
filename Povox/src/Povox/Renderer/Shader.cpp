#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Shader.h"


#include "Platform/Vulkan/VulkanShader.h"



namespace Povox {

	Ref<Shader> Shader::Create(const std::string& filepath)
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



	//------------------ShaderLibrary---------------------
	
	ShaderLibrary::ShaderLibrary()
	{
		PX_CORE_TRACE("Created ShaderLibrary!");
	}

	void ShaderLibrary::Shutdown()
	{
		PX_CORE_INFO("ShaderLibrary::Shutdown: Starting...");

		for (auto& shader : m_Shaders)
		{
			shader.second->Free();
		}


		PX_CORE_INFO("ShaderLibrary::Shutdown: Completed.");
	}

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		if (Contains(name)) 
			PX_CORE_WARN("Shader '{0}' already exists!", name);
		else
		{
			m_Shaders[name] = shader;
			PX_CORE_TRACE("Added shader '{0}'", name);
		}
	}
	
	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto name = shader->GetDebugName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);

		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);

		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		PX_CORE_ASSERT(Contains(name), "Shader does not exist!");

		return m_Shaders[name];
	}

	bool ShaderLibrary::Contains(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

}
