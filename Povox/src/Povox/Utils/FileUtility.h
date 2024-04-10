#pragma once
#include "Povox/Core/Log.h"


#include <string>
#include <filesystem>
#include <fstream>


namespace Povox { namespace Utils {

	namespace Shader {

		static std::string ReadFile(const std::filesystem::path path)
		{
			std::string result;
			// input file stream, binary, cause we don't want to change something here. just load it
			std::ifstream in(path, std::ios::in | std::ios::binary);
			if (in)
			{
				in.seekg(0, std::ios::end);
				size_t size = in.tellg();
				if (size != -1)
				{
					result.resize(size);
					in.seekg(0, std::ios::beg);
					in.read(&result[0], size);
				}
				else
				{
					PX_CORE_ERROR("Could not read from file {0}", path);
				}								// close the stream
			}
			else
			{
				PX_CORE_ERROR("Could not open file '{0}' !", path);
			}

			return result;
		}
		static std::string ReadFile(const std::string& filepath)
		{
			std::string result;
			// input file stream, binary, cause we don't want to change something here. just load it
			std::ifstream in(filepath, std::ios::in | std::ios::binary);
			if (in)
			{
				in.seekg(0, std::ios::end);
				size_t size = in.tellg();
				if (size != -1)
				{
					result.resize(size);
					in.seekg(0, std::ios::beg);
					in.read(&result[0], size);
				}
				else
				{
					PX_CORE_ERROR("Could not read from file {0}", filepath);
				}								// close the stream
			}
			else
			{
				PX_CORE_ERROR("Could not open file '{0}' !", filepath);
			}

			return result;
		}

		static const char* GetGLCacheDirectory()
		{
			return "assets/cache/shader/opengl";
		}

		static const char* GetVKCacheDirectory()
		{
			return "assets/cache/shader/vulkan";
		}

		static void CreateGLCacheDirectoryIfNeeded()
		{
			std::string path = GetGLCacheDirectory();
			if (!std::filesystem::exists(path))
				std::filesystem::create_directories(path);
		}

		static void CreateVKCacheDirectoryIfNeeded()
		{
			std::string path = GetVKCacheDirectory();
			if (!std::filesystem::exists(path))
				std::filesystem::create_directories(path);
		}

	}

	static std::string GetFileExtension(const std::string filepath)
	{
		std::string extension;

		const char* dot = ".";
		size_t pos = filepath.find_first_of(dot, 0);

		if (pos != std::string::npos)
		{
			size_t begin = pos + 1;
			extension = filepath.substr(begin, std::string::npos - begin);
		}
		return extension;
	}


} }
