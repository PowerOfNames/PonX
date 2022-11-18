/*#pragma once
#include "Povox/Renderer/Shader.h"

#include <glm/glm.hpp>

// TODO: REMOVE!
typedef unsigned int GLenum;

namespace Povox
{
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		~OpenGLShader();


		virtual const std::string& GetName() const override { return m_Name; }
		
	// Uniforms
		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;

		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, const glm::vec2& vector) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& vector) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& vector) override;

		virtual void SetMat3(const std::string& name, const glm::mat3& matrix) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& matrix) override;


		virtual void UploadUniformInt(const std::string& name, int value);
		virtual void UploadUniformIntArray(const std::string& name, int* value, uint32_t count);
		
		virtual void UploadUniformFloat(const std::string& name, float value);
		virtual void UploadUniformFloat2(const std::string& name, const glm::vec2& vector);
		virtual void UploadUniformFloat3(const std::string& name, const glm::vec3& vector);
		virtual void UploadUniformFloat4(const std::string& name, const glm::vec4& vector);

		virtual void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		virtual void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);

	private:
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);

		void CompileOrGetVulkanBinaries(std::unordered_map<GLenum, std::string>& shaderSources);
		void CompileOrGetOpenGLBinaries();
		void CreateProgram();
		void Reflect(GLenum stage, const std::vector<uint32_t>& shaderData);

	private:
		uint32_t m_RendererID;
		std::string m_Name;
		std::string m_FilePath;

		std::unordered_map<GLenum, std::vector<uint32_t>> m_VulkanSpirV;
		std::unordered_map<GLenum, std::vector<uint32_t>> m_OpenGLSpirV;

		std::unordered_map<GLenum, std::string> m_OpenGLShaderSourceCode;
	};
}*/
