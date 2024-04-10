#pragma once
#include "Povox/Renderer/Shader.h"


#include "FileWatch.h"


namespace Povox {

	struct Hash_128Bit
	{
		uint64_t Low64;
		uint64_t High64;

		bool operator==(const Hash_128Bit& other) const
		{
			return (Low64 == other.Low64 && High64 == other.High64);
		}
	};

	struct ShaderMetaData
	{
		ShaderHandle Handle = 0;
		std::string DebugName;

		// during editor -> path links shader to the .glsl raw shader code
		
		// during runtime -> path links shader to the compiled .spv binary file that is cached? Maybe in a compressed format...
		// during runtime -> shader handles are stored with some metadata inside a json or similar inside a ShaderPack containing all shaders needed for the game (maybe changeable using dif properties)
		// Material contains shader handle? Shader somehow links to pipeline, so when different objects get processes, their material is used to decide which pipelines are needed?
		//		->	for now, dont do this dynamically (if ever) it should be enough to let the renderers handle the sorting and structuring of object rendering, such that the backend
		//			only needs to linearly (roughly, render graph will come at some point)
		std::filesystem::path Path;
		Hash_128Bit ContentHash = { 0, 0 };

		Ref<Shader> Shader;
	};

	using ShaderRegistry = std::unordered_map<ShaderHandle, ShaderMetaData>;

	class ShaderManager
	{
	public:
		ShaderManager(const std::filesystem::path& fileSystemShadersPath);
		~ShaderManager() = default;

		void Shutdown();

		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);

		Ref<Shader> Load(const std::string& name, const std::string& fileSystemShadersPath);
		ShaderHandle Load(const std::filesystem::path& fileName);

		Ref<Shader> Get(const std::string& name) const;
		Ref<Shader> Get(ShaderHandle handle) const;

		bool Contains(const std::string& name) const;


	private:
		void InitFilewatcher();
		void OnShaderFileWatchEvent(const std::wstring& path, const filewatch::Event change_type);
		void RecompileShader(const std::filesystem::path& path);
		void RecompileShader(const std::string& shaderName);
		void RecompileShader(ShaderHandle handle);

		void RenameShader(ShaderHandle handle, const std::string& newName);

	private:
		ShaderRegistry m_Registry;
		// temp
		std::unordered_map<std::string, ShaderHandle> m_NameToHandle;

		std::unordered_map<std::string, Ref<Shader>> m_Shaders;

		std::filesystem::path m_FileSystemShadersPath;


		Scope<filewatch::FileWatch<std::wstring>> m_FileWatcher;
		bool m_FileWatcherPending = false;
		std::string m_RenamedOldName;
	};
}
