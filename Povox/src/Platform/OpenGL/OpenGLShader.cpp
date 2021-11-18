#include "pxpch.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "Povox/Core/Log.h"
#include "Povox/Utils/FileUtility.h"

#include <fstream>
#include <filesystem>
#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "Povox/Core/Time.h"

namespace Povox {

	namespace Utils {
		namespace Shader {

			static GLenum GLShaderTypeFromString(const std::string& name)
			{
				if (name == "vertex")
					return GL_VERTEX_SHADER;
				if (name == "fragment" || name == "pixel")
					return GL_FRAGMENT_SHADER;

				PX_CORE_ASSERT(false, "Shadertype not defined");
				return 0;
			}

			static shaderc_shader_kind GLShaderStageToShaderC(GLenum stage)
			{
				switch (stage)
				{
				case GL_VERTEX_SHADER: return shaderc_glsl_vertex_shader;
				case GL_FRAGMENT_SHADER: return shaderc_glsl_fragment_shader;
				}

				PX_CORE_ASSERT(false, "Shaderstage not defined");
				return (shaderc_shader_kind)0;
			}

			static const char* GLShaderStageToString(GLenum stage)
			{
				switch (stage)
				{
				case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
				case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
				}

				PX_CORE_ASSERT(false, "Shaderstage not defined");
				return nullptr;
			}

			static const char* GLShaderStageCachedOpenGLFileExtension(GLenum stage)
			{
				switch (stage)
				{
				case GL_VERTEX_SHADER: return ".chached_opengl.vert";
				case GL_FRAGMENT_SHADER: return ".chached_opengl.frag";
				}
				PX_CORE_ASSERT(false, "Shaderstage not defined");
				return "";
			}

			static const char* GLShaderStageCachedVulkanFileExtension(GLenum stage)
			{
				switch (stage)
				{
				case GL_VERTEX_SHADER: return ".chached_vulkan.vert";
				case GL_FRAGMENT_SHADER: return ".chached_vulkan.frag";
				}
				PX_CORE_ASSERT(false, "Shaderstage not defined");
				return "";
			}
		}
	}
	

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		PX_PROFILE_FUNCTION();


		Utils::Shader::CreateGLCacheDirectoryIfNeeded();

		PX_CORE_WARN("Starting to load shader from '{0}'", filepath);
		std::string sources = Utils::Shader::ReadFile(filepath);
		auto shaderSources = PreProcess(sources);

		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSources);
			CompileOrGetOpenGLBinaries();
			CreateProgram();
			PX_CORE_WARN("Shader took {0}ms", timer.ElapsedMilliseconds());
		}


		// Extract name from filepath
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind(".");
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
		: m_Name(name)
	{
		PX_PROFILE_FUNCTION();


		std::unordered_map<GLenum, std::string> shaderSources;
		shaderSources[GL_VERTEX_SHADER] = vertexSrc;
		shaderSources[GL_FRAGMENT_SHADER] = fragmentSrc;

		{
			Timer timer;
			CompileOrGetVulkanBinaries(shaderSources);
			CompileOrGetOpenGLBinaries();
			CreateProgram();
			PX_CORE_WARN("Shader took {0}ms", timer.ElapsedMilliseconds());
		}
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteShader(m_RendererID);
	}

	void OpenGLShader::CompileOrGetVulkanBinaries(std::unordered_map<GLenum, std::string>& shaderSources)
	{
		GLuint program = glCreateProgram();
		
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::Shader::GetGLChacheDirectory();

		auto& shaderData = m_VulkanSpirV;
		shaderData.clear();
		for (auto&& [stage, code] : shaderSources)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::Shader::GLShaderStageCachedVulkanFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open()) // happens if chache path exists
			{
				in.seekg(0, std::ios::end);				// places pointer to end of file
				auto size = in.tellg();					// position of pointer equals size
				in.seekg(0, std::ios::beg);				// places pointer back to start

				auto& data = shaderData[stage];			// gets a vector of uint32_t
				data.resize(size / sizeof(uint32_t));	// sets the size of the vector
				in.read((char*)data.data(), size);		// reads in and writes to data I guess
			}
			else
			{		
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(code, Utils::Shader::GLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					PX_CORE_ERROR(module.GetErrorMessage());
					PX_CORE_ASSERT(false, module.GetErrorMessage());
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}

		for (auto&& [stage, data] : shaderData)
		{
			Reflect(stage, data);
		}
	}

	void OpenGLShader::CompileOrGetOpenGLBinaries()
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::Shader::GetGLChacheDirectory();

		auto& shaderData = m_OpenGLSpirV;
		shaderData.clear();
		m_OpenGLShaderSourceCode.clear();
		for (auto&& [stage, spirv] : m_VulkanSpirV)
		{
			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::Shader::GLShaderStageCachedOpenGLFileExtension(stage));


			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open())							// happens if chache path exists
			{
				in.seekg(0, std::ios::end);				// places pointer to end of file
				auto size = in.tellg();					// position of pointer equals size
				in.seekg(0, std::ios::beg);				// places pointer back to start

				auto& data = shaderData[stage];			// gets a vector of uint32_t
				data.resize(size / sizeof(uint32_t));	// sets the size of the vector
				in.read((char*)data.data(), size);		// reads in and writes to data I guess
			}
			else
			{
				spirv_cross::CompilerGLSL glslCompiler(spirv);
				m_OpenGLShaderSourceCode[stage] = glslCompiler.compile();
				auto& source = m_OpenGLShaderSourceCode[stage];

				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::Shader::GLShaderStageToShaderC(stage), m_FilePath.c_str());
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					PX_CORE_ERROR(module.GetErrorMessage());
					PX_CORE_ASSERT(false, module.GetErrorMessage());
				}

				shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = shaderData[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
	}

	void OpenGLShader::CreateProgram()
	{
		GLuint program = glCreateProgram();

		std::vector<GLint> shaderIDs;
		for (auto&& [stage, spirv] : m_OpenGLSpirV)
		{
			GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
			glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size() * sizeof(uint32_t));
			glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
			glAttachShader(program, shaderID);
		}

		glLinkProgram(program);

		GLint isLinked;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
			PX_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_FilePath, infoLog.data());

			glDeleteProgram(program);

			for (auto id : shaderIDs)
				glDeleteShader(id);
		}

		for (auto id : shaderIDs)
		{
			glDetachShader(program, id);
			glDeleteShader(id);
		}

		m_RendererID = program;

	}

	void OpenGLShader::Reflect(GLenum stage, const std::vector<uint32_t>& shaderData)
	{
		spirv_cross::Compiler compiler(shaderData);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		PX_CORE_TRACE("OpenGLShader::Reflect - {0} {1}", Utils::Shader::GLShaderStageToString(stage), m_FilePath);
		PX_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
		PX_CORE_TRACE("    {0} resources", resources.sampled_images.size());

		PX_CORE_TRACE("Uniform buffers:");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			int memberCount = bufferType.member_types.size();

			PX_CORE_TRACE("  {0}", resource.name);
			PX_CORE_TRACE("    Size = {0}", bufferSize);
			PX_CORE_TRACE("    Binding = {0}", binding);
			PX_CORE_TRACE("    Members = {0}", memberCount);
		}
	}

	// in addition to the current code, I should make sure, that additional 'spaces' in front of the type name or so get cut out
	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{
		PX_PROFILE_FUNCTION();


		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			// finds end of line AFTER the token
			// sets the begin of the 'type' to one after '#type'
			// cuts the type from the string

			size_t eol = source.find_first_of("\r\n", pos);
			PX_CORE_ASSERT(eol != std::string::npos, "Syntax Error!");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			PX_CORE_ASSERT(Utils::Shader::GLShaderTypeFromString(type), "Invalid shader type specified!");

			// finds the beginning of the shader string
			// sets position of the next '#type' token
			// cuts the shader code up until the next token or the end of the whole file
			size_t nextLine = source.find_first_not_of("\r\n", eol);
			PX_CORE_ASSERT(nextLine != std::string::npos, "Syntax Error!");
			pos = source.find(typeToken, nextLine);

			shaderSources[Utils::Shader::GLShaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLine) : source.substr(nextLine, pos - nextLine);
		}
		return shaderSources;
	}

	void OpenGLShader::Bind() const
	{
		PX_PROFILE_FUNCTION();


		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		PX_PROFILE_FUNCTION();


		glUseProgram(0);
	}

	void OpenGLShader::SetInt(const std::string& name, int value)
	{ 
		UploadUniformInt(name, value); 
	}

	void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count)
	{
		UploadUniformIntArray(name, values, count);
	}

	void OpenGLShader::SetFloat(const std::string& name, float value)
	{
		UploadUniformFloat(name, value);
	}

	void OpenGLShader::SetFloat2(const std::string& name, const glm::vec2& vector)
	{
		UploadUniformFloat2(name, vector);
	}

	void OpenGLShader::SetFloat3(const std::string& name, const glm::vec3& vector)
	{
		UploadUniformFloat3(name, vector);
	}
	void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& vector)
	{ 
		UploadUniformFloat4(name, vector); 
	}

	void OpenGLShader::SetMat3(const std::string& name, const glm::mat3& matrix)
	{ 
		UploadUniformMat4(name, matrix); 
	}
	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& matrix)
	{ 
		UploadUniformMat4(name, matrix); 
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& name, int* values, uint32_t count)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& vector)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniform2f(location, vector.x, vector.y);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& vector)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_WARN("Uniform '{0}' not in shader!", name);
		glUniform3f(location, vector.x, vector.y, vector.z);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& vector)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniform4f(location, vector.x, vector.y, vector.z, vector.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!")
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}
}
