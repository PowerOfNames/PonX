#pragma once
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

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual const std::string& GetName() const override { return m_Name; }
		
	// Uniforms
		virtual void SetInt(const std::string& name, int value) override;

		virtual void SetFloat3(const std::string& name, const glm::vec3& vector) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& vector) override;

		virtual void SetMat3(const std::string& name, const glm::mat3& matrix) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& matrix) override;


		virtual void UploadUniformInt(const std::string& name, int value);
		
		virtual void UploadUniformFloat(const std::string& name, float value);
		virtual void UploadUniformFloat2(const std::string& name, const glm::vec2& vector);
		virtual void UploadUniformFloat3(const std::string& name, const glm::vec3& vector);
		virtual void UploadUniformFloat4(const std::string& name, const glm::vec4& vector);

		virtual void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		virtual void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);

	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
		void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);

	private:
		uint32_t m_RendererID;
		std::string m_Name;
	};
}
