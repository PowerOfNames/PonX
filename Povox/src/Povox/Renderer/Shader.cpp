#include "pxpch.h"
#include "Povox/Renderer/Shader.h"

#include "Povox/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/Vulkan/VulkanShader.h"



namespace Povox {

	Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::API::OpenGL:
			{
				return CreateRef<OpenGLShader>(filepath);
			}
			case RendererAPI::API::Vulkan:
			{
				return CreateRef<VulkanShader>(filepath);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::NONE:
			{
				PX_CORE_ASSERT(false, "RendererAPI::NONE is not supported!");
				return nullptr;
			}
			case RendererAPI::API::OpenGL:
			{
				return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
			}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		if (Contains(name)) 
			PX_CORE_INFO("Shader '{0}' already exists!", name);
		else
			m_Shaders[name] = shader;
	}
	
	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto name = shader->GetName();
		Add(name, shader);
	}

	Ref<Povox::Shader> ShaderLibrary::Load(const std::string& name, std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);

		return shader;
	}

	Ref<Povox::Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(shader);

		return shader;
	}

	Ref<Povox::Shader> ShaderLibrary::Get(const std::string& name)
	{
		PX_CORE_ASSERT(Contains(name), "Shader does not exist!");
		return m_Shaders[name];
	}

	bool ShaderLibrary::Contains(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

}
