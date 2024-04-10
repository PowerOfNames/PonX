#include "pxpch.h"

#include "Povox/Core/Application.h"
#include "Povox/Resources/ShaderManager.h"


#include "xxHash.h"
#include <codecvt>

#include "Povox/Utils/FileUtility.h"



namespace Povox {

	//------------------ShaderLibrary---------------------
	ShaderManager::ShaderManager(const std::filesystem::path& fileSystemShadersPath)
		: m_FileSystemShadersPath(fileSystemShadersPath)
	{
		InitFilewatcher();


		PX_CORE_TRACE("Created ShaderLibrary!");
	}

	void ShaderManager::Shutdown()
	{
		PX_CORE_INFO("ShaderLibrary::Shutdown: Starting...");

		for (auto& shader : m_Shaders)
		{
			shader.second->Free();
		}


		PX_CORE_INFO("ShaderLibrary::Shutdown: Completed.");
	}

	void ShaderManager::Add(const std::string& name, const Ref<Shader>& shader)
	{
		if (Contains(name))
			PX_CORE_WARN("Shader '{0}' already exists!", name);
		else
		{
			m_Shaders[name] = shader;
			PX_CORE_TRACE("Added shader '{0}'", name);
		}
	}

	void ShaderManager::Add(const Ref<Shader>& shader)
	{
		auto name = shader->GetDebugName();
		Add(name, shader);
	}

	Ref<Shader> ShaderManager::Load(const std::string& name, const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		Add(name, shader);

		return shader;
	}

	ShaderHandle ShaderManager::Load(const std::filesystem::path& shaderName)
	{	
		PX_CORE_WARN("File name: {}", shaderName.string());
		PX_CORE_WARN("File name: {}", shaderName.extension().string());
		if (shaderName.extension().string() != ".glsl")
		{
			PX_CORE_WARN("File is no shader");
			return 0;
		}

		ShaderMetaData meta{};
		meta.DebugName = shaderName.stem().string();
		meta.Path = m_FileSystemShadersPath / shaderName;

		std::string sources = Utils::Shader::ReadFile(m_FileSystemShadersPath / shaderName);
		std::vector<char> cstr(sources.c_str(), sources.c_str() + sources.size() + 1);
		auto hash = XXH3_128bits((const void*)cstr.data(), cstr.size());
		meta.ContentHash.Low64 = hash.low64;
		meta.ContentHash.High64 = hash.high64;

		meta.Shader = Shader::Create(sources, meta.DebugName);
		ShaderHandle handle = meta.Shader->GetID();
		Add(meta.Shader);
		m_Registry[handle] = meta;
		m_NameToHandle[meta.DebugName] = handle;

		return handle;
	}

	Ref<Shader> ShaderManager::Get(ShaderHandle handle) const
	{
		if (m_Registry.find(handle) == m_Registry.end())
			return nullptr;

		return m_Registry.at(handle).Shader;
	}

	Ref<Shader> ShaderManager::Get(const std::string& name) const
	{
		PX_CORE_ASSERT(Contains(name), "Shader does not exist!");

		return m_Shaders.at(name);
	}

	bool ShaderManager::Contains(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

	void ShaderManager::InitFilewatcher()
	{
		m_FileWatcher = CreateScope<filewatch::FileWatch<std::wstring>>(m_FileSystemShadersPath,
			[this](const std::wstring& changedFile, const filewatch::Event changeType) { OnShaderFileWatchEvent(changedFile, changeType); });
		m_FileWatcherPending = false;
	}

	void ShaderManager::OnShaderFileWatchEvent(const std::wstring& changedFile, const filewatch::Event changeType)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		const std::string fileNameString = converter.to_bytes(changedFile);
		

		if (m_FileWatcherPending)
			return;
		m_FileWatcherPending = true;

		std::string name = std::filesystem::path(fileNameString).stem().string();
		if (m_NameToHandle.find(name) == m_NameToHandle.end())
		{
			PX_CORE_WARN("Renamed Shader that was not registered: {}", fileNameString);
			return;
		}
		ShaderHandle handle = m_NameToHandle.at(name);

		switch (changeType)
		{
			case filewatch::Event::added:
			{
				Application::Get()->AddMainThreadQueueInstruction([this, fileNameString]()
					{
						m_FileWatcher.reset();

						PX_CORE_WARN("Added: {}", fileNameString);

						InitFilewatcher();
					});
				break;
			}
			case filewatch::Event::removed:
			{
				Application::Get()->AddMainThreadQueueInstruction([this, fileNameString]()
					{
						m_FileWatcher.reset();

						PX_CORE_WARN("Removed: {}", fileNameString);

						InitFilewatcher();
					});
				break;
			}
			case filewatch::Event::renamed_new:
			{
				if (name == m_RenamedOldName)
				{
					Application::Get()->AddMainThreadQueueInstruction([this, handle]()
						{
							m_FileWatcher.reset();

							RecompileShader(handle);

							InitFilewatcher();
						});
					m_RenamedOldName = std::string();
				}
				else
				{
					Application::Get()->AddMainThreadQueueInstruction([this, handle, name]()
						{
							m_FileWatcher.reset();


							InitFilewatcher();
						});
				}
				break;
			}
			case filewatch::Event::renamed_old:
			{				
				m_RenamedOldName = name;

				Application::Get()->AddMainThreadQueueInstruction([this]()
					{
						m_FileWatcher.reset();


						InitFilewatcher();
					});
				break;
			}
			case filewatch::Event::modified:
			{
				Application::Get()->AddMainThreadQueueInstruction([this, handle]()
					{
						m_FileWatcher.reset();

						PX_CORE_WARN("RecompileShader:: Recompiled shader {}!", handle);
						RecompileShader(handle);

						InitFilewatcher();
					});
				break;
			}
			default:
				PX_CORE_WARN("FileWatch::EventType unknown!");
		}
	}


	void ShaderManager::RecompileShader(const std::filesystem::path& path)
	{
		if (!Contains(path.filename().string()))
			PX_CORE_WARN("Shader {} not registered!", path.filename().string());

		PX_CORE_WARN("Manipulated shader: {}", path.filename().string().c_str());

		RecompileShader(path.filename().string());
	}
	void ShaderManager::RecompileShader(const std::string& shaderName)
	{

	}
	void ShaderManager::RecompileShader(ShaderHandle handle)
	{
		if (m_Registry.find(handle) == m_Registry.end())
		{
			PX_CORE_WARN("RecompileShader:: Shader {} not registered!", handle);
			return;
		}
		
		ShaderMetaData& meta = m_Registry.at(handle);
		std::string sources = Utils::Shader::ReadFile(meta.Path);
		std::vector<char> cstr(sources.c_str(), sources.c_str() + sources.size() + 1);
		auto hash = XXH3_128bits((const void*)cstr.data(), cstr.size());
		Hash_128Bit newHash{hash.low64, hash.high64};
		if (meta.ContentHash == newHash)
			return;

		// if recompilation succeeded:
		if(meta.Shader->Recompile(sources))
			meta.ContentHash = newHash;
	}

	void ShaderManager::RenameShader(ShaderHandle handle, const std::string& newName)
	{
		std::string oldName;
		if (m_Registry.find(handle) != m_Registry.end())
		{
			auto& meta = m_Registry.at(handle);
			oldName = meta.DebugName;
			meta.DebugName = newName;
			meta.Shader->Rename(newName);
		}		
		m_NameToHandle.erase(oldName);
		m_NameToHandle[newName] = handle;		
	}
}
