#include "pxpch.h"
#include "Povox/Renderer/Renderer.h"
#include "Povox/Renderer/Shader.h"

#include "Platform/Vulkan/VulkanShader.h"

#include "Povox/Core/Application.h"
#include "Povox/Core/Core.h"
#include "Povox/Core/Log.h"

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


	Scope<filewatch::FileWatch<std::wstring>> ShaderLibrary::s_FileWatcher = nullptr;
	bool ShaderLibrary::s_ShaderLibraryReloadPending = false;

	//------------------ShaderLibrary---------------------
	ShaderLibrary::ShaderLibrary(const std::filesystem::path& fileSystemShadersPath)
		: m_FileSystemShadersPath(fileSystemShadersPath)
	{
		PX_CORE_TRACE("Created ShaderLibrary!");
		
		s_FileWatcher = CreateScope<filewatch::FileWatch<std::wstring>>(m_FileSystemShadersPath, OnShaderFileWatchEvent);
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

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
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

	void ShaderLibrary::OnShaderFileWatchEvent(const std::wstring& path, const filewatch::Event changeType)
	{
		if (!s_ShaderLibraryReloadPending)
		{
			s_ShaderLibraryReloadPending = true;
			switch (changeType)
			{
				case filewatch::Event::added:
				{
					Application::Get()->AddMainThreadQueueInstruction([](ShaderLibrary* lib)
						{
							s_FileWatcher.reset();
							Add(path)
						});
					break;
				}
				case filewatch::Event::removed:
				{

					break;
				}
				case filewatch::Event::renamed_new:
				{

					break;
				}
				case filewatch::Event::renamed_old:
				{

					break;
				}
				case filewatch::Event::modified:
				{

					break;
				}
				default:
					PX_CORE_WARN("FileWatch::EventType unknown!");
			}
		}		
	}

}
