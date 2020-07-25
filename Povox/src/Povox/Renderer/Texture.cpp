#include "pxpch.h"
#include "Povox/Renderer/Texture.h"

#include "Povox/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

#include "Povox/Utility/Misc.h"

namespace Povox {

	Povox::Ref<Povox::Texture2D> Texture2D::Create(const std::string& name, uint32_t width, uint32_t height)
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
			return CreateRef<OpenGLTexture2D>(name, width, height);
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path)
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
			return CreateRef<OpenGLTexture2D>(path);
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& name, const std::string& path)
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
			return CreateRef<OpenGLTexture2D>(name, path);
		}
		}
		PX_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void Texture2DLibrary::Add(const std::string& name, const Ref<Texture2D>& texture)
	{
		if (!Contains(name))
			m_Textures[name] = texture;
	}

	void Texture2DLibrary::Add(const Ref<Texture2D>& texture)
	{
		auto name = texture->GetName();
		Add(name, texture);
	}

	Povox::Ref<Povox::Texture2D> Texture2DLibrary::Load(const std::string& name, uint32_t width, uint32_t height)
	{
		auto texture = Texture2D::Create(name, width, height);
		Add(name, texture);

		return m_Textures[name];
	}

	Ref<Povox::Texture2D> Texture2DLibrary::Load(const std::string& name, std::string& filepath)
	{
		auto texture = Texture2D::Create(filepath);
		Add(name, texture);

		return m_Textures[name];
	}

	Ref<Povox::Texture2D> Texture2DLibrary::Load(const std::string& filepath)
	{
		auto texture = Texture2D::Create(filepath);
		std::string name = texture->GetName();
		Add(name, texture);

		return m_Textures[name];
	}

	Ref<Povox::Texture2D> Texture2DLibrary::Get(const std::string& name)
	{
		PX_CORE_ASSERT(Contains(name), "Texture does not exist!");
		return m_Textures[name];
	}

	bool Texture2DLibrary::Contains(const std::string& name)
	{
		return m_Textures.find(name) != m_Textures.end();
	}
}