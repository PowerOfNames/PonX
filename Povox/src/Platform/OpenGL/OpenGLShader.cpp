#include "pxpch.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "Povox/Core/Log.h"

#include <fstream>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Povox {

	static GLenum ShaderTypeFromString(const std::string& name)
	{
		if (name == "vertex") 
			return GL_VERTEX_SHADER;		
		if (name == "fragment" || name == "pixel")
			return GL_FRAGMENT_SHADER;

		PX_CORE_ASSERT(false, "Shadertype not defined");
		return 0;
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		std::string shaderSources = ReadFile(filepath);
		Compile(PreProcess(shaderSources));
	}

	OpenGLShader::OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		std::unordered_map<GLenum, std::string> shaderSources;
		shaderSources[GL_VERTEX_SHADER] = vertexSrc;
		shaderSources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(shaderSources);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteShader(m_RendererID);
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::Unbind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!");
		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!");
		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& vector)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!");
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
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!");
		glUniform4f(location, vector.x, vector.y, vector.z, vector.w);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!");
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location == -1)
			PX_CORE_ASSERT(false, "Uniform '" + name + "' not in shader!");
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	std::string OpenGLShader::ReadFile(const std::string& filepath)
	{
		std::string result;
		// input file stream, binary, cause we don't want to change something here. just load it
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);					// places pointer to end of file
			result.resize(in.tellg());					// position of pointer equals size
			in.seekg(0, std::ios::beg);					// places pointer back to start
			in.read(&result[0], result.size());			// reads in the content of the file 
			in.close();									// close the stream
		}
		else
		{
			PX_CORE_ERROR("Could not open file '{0}' !", filepath);
		}

		return result;
	}


	// in addition to the current code, I should make sure, that additional 'spaces' in front of the type name or so get cut out
	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{
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
			PX_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified!");

			// finds the beginning of the shader string
			// sets position of the next '#type' token
			// cuts the shader code up until the next token or the end of the whole file
			size_t nextLine = source.find_first_not_of("\r\n", eol);
			PX_CORE_ASSERT(nextLine != std::string::npos, "Syntax Error!");
			pos = source.find(typeToken, nextLine);
			shaderSources[ShaderTypeFromString(type)] = source.substr(nextLine, pos - (nextLine == std::string::npos ? source.size() - 1 : nextLine));
		}
		return shaderSources;
	}

	void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string>& shaderSources)
	{
		GLuint program = glCreateProgram();
		PX_CORE_ASSERT(shaderSources.size() <= 2, "We only support 2 shaders for now!");
		std::array<GLenum, 2> glShaderIDs;
		int shaderIdIndex = 0;

		// For each enum type in 'shaderSources' create and bind the corresponding shader to the program
		for (auto&& [key, value] : shaderSources)
		{
			// Create an empty vertex shader handle
			GLuint shader = glCreateShader(key);

			// Send the vertex shader source code to GL
			// Note that std::string's .c_str is NULL character terminated.
			const GLchar* source = value.c_str();
			glShaderSource(shader, 1, &source, 0);

			// Compile the vertex shader
			glCompileShader(shader);

			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				// The maxLength includes the NULL character
				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

				// We don't need the shader anymore.
				glDeleteShader(shader);

				PX_CORE_ERROR("{0}", infoLog.data());
				PX_CORE_ASSERT(false, "Shader compilation failure!");
				break;
			}

			// Attach our shaders to our program
			glAttachShader(program, shader);
			glShaderIDs[shaderIdIndex++] = shader;
		}
		m_RendererID = program;

		// Link our program
		glLinkProgram(program);
		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(program);
			// Don't leak shaders either.
			for (auto id : glShaderIDs)
				glDeleteShader(id);

			PX_CORE_ERROR("{0}", infoLog.data());
			PX_CORE_ASSERT(false, "Shader program linking failure!");

			return;
		}

		// Always detach shaders after a successful link.
		for (auto id : glShaderIDs)
		{
			glDetachShader(program, id);
			glDeleteShader(id);
		}
	}



}