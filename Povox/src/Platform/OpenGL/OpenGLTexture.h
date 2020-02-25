#pragma once
#include "Povox/Renderer/Texture.h"

#include <glad/glad.h>

#include <string>

namespace Povox {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const std::string& name, uint32_t width, uint32_t height);
		OpenGLTexture2D(const std::string& path);
		OpenGLTexture2D(const std::string& name, const std::string& path);
		virtual ~OpenGLTexture2D();

		inline virtual uint32_t GetWidth() const override { return m_Width; }
		inline virtual uint32_t GetHeight() const override { return m_Height; }

		inline virtual const std::string& GetName() const override { return m_Name; }

		virtual void SetData(void* data, uint32_t size) override;

		virtual void Bind(uint32_t slot = 0) const override;

		inline const uint32_t GetId() { return m_RendererID; };

	private:
		void Compile(const std::string& filepath);
	private:
		std::string m_Path;
		std::string m_Name;
		uint32_t m_Width, m_Height;

		uint32_t m_RendererID;
		GLuint m_InternalFormat, m_DataFormat;
	};

}