#include "pxpch.h"
#include "Povox/Renderer/TextureSystem.h"

#include "Povox/Core/Log.h"

namespace Povox {


	TextureSystem::TextureSystem()
	{
		Init();
	}


	TextureSystem::~TextureSystem()
	{
		Shutdown();
	}


	void TextureSystem::Init()
	{
		PX_PROFILE_FUNCTION();

		m_SystemState.Config.TexturesPath = "assets/textures/";
		m_SystemState.RegisteredTextures.reserve(m_SystemState.Config.MaxTextures);

		CreateDefaultTexture();
	}


	void TextureSystem::Shutdown()
	{
		// TODO: Clean all textures upon shutdown by calling destroy texture? Or let smart pointers handle the cleanup -> clear the pixel array(s)
		/*for (auto& texture : s_SystemState.m_RegisteredTextures)
		{
			texture.second->Destroy();
		}*/
	}

	// TODO: Alternatively, use the whole path here instead of name in case textures exist in multiple files
	Ref<Texture> TextureSystem::RegisterTexture(const std::string& name)
	{
		Ref<Texture> texture = RegisteredTexturesContains(name);
		if (texture)
			return texture;


		// TODO: query the ending from texture format/type | Check if this returns null if path is wrong
		texture = Texture2D::Create(m_SystemState.Config.TexturesPath + name + ".png");
		if (!texture)
		{
			PX_CORE_WARN("TextureSystem::RegisterTexture: Failed to register texture '{0}'. Returning DefaultTexture if set!", name);
			return m_SystemState.DefaultTexture != nullptr ? m_SystemState.DefaultTexture : nullptr;
		}
		m_SystemState.RegisteredTextures[name] = texture;

		return texture;
	}

	Ref<Texture> TextureSystem::RegisterTexture(const std::string& name, Ref<Texture> newTexture)
	{
		Ref<Texture> texture = RegisteredTexturesContains(name);
		if (texture)
		{
			PX_CORE_WARN("TextureSystem::RegisterTexture: Name '{0}' already registered ({1}). Adding '_duplicateName' to the name!", name, RegisteredTexturesContains(texture));
			const std::string newName = name + "_duplicateName";
			m_SystemState.RegisteredTextures[newName] = newTexture;
			return newTexture;
		}
		m_SystemState.RegisteredTextures[name] = newTexture;
		return newTexture;
	}

	Ref<Texture> TextureSystem::GetTexture(const std::string& name)
	{
		if (RegisteredTexturesContains(name))
			return m_SystemState.RegisteredTextures[name];

		PX_CORE_WARN("TextureSystem::GetTexture: Texture '{0}' not found. Attempting to register from '{1}{0}.png'", name, m_SystemState.Config.TexturesPath);
		
		return RegisterTexture(name);
	}


	const uint64_t TextureSystem::BindTexture(const std::string& name)
	{
		Ref<Texture> texture = RegisteredTexturesContains(name);
		return BindTexture(texture);
	}

	const uint64_t TextureSystem::BindTexture(Ref<Texture> texture)
	{
		PX_CORE_ASSERT(texture, "Tried to bind unknown texture! Register Texture before please!");
		for (uint32_t i = 0; i < m_SystemState.NextTextureSlot; i++)
		{
			PX_CORE_WARN("TextureSystem::BindTexture: i = {0}, nextSlot = {1}", i, m_SystemState.NextTextureSlot);
			if (*texture.get() == *m_SystemState.ActiveTextures[i].get())
			{
				m_SystemState.ActiveTexturesCounter[i]++;
				return i;
			}
		}

		if (m_SystemState.NextTextureSlot +1 == MAX_TEXTURE_SLOTS)
		{
			PX_CORE_WARN("TextureSystem::BindTexture: Max_Texture_Slots reached. Pls draw and reset!");
			return MAX_TEXTURE_SLOTS + 1;
		}

		m_SystemState.ActiveTextures[m_SystemState.NextTextureSlot] = texture;
		m_SystemState.ActiveTexturesCounter[m_SystemState.NextTextureSlot] = 1;
		m_SystemState.NextTextureSlot++;


		return m_SystemState.NextTextureSlot;
	}


	void TextureSystem::ResetActiveTextures()
	{
		m_SystemState.ActiveTextures;
		m_SystemState.ActiveTexturesCounter;
		m_SystemState.NextTextureSlot = 1; // 1, so the white texture can stay in slot 0
	}

	void TextureSystem::CreateDefaultTexture()
	{
		m_SystemState.DefaultTexture = RegisterTexture("DefaultPXTexture");
		PX_CORE_ASSERT(m_SystemState.DefaultTexture, "No Default Texture was set!");
	}


	Ref<Texture> TextureSystem::RegisteredTexturesContains(const std::string& name)
	{
		if (m_SystemState.RegisteredTextures.find(name) == m_SystemState.RegisteredTextures.end())
			return nullptr;
		return m_SystemState.RegisteredTextures[name];
	}


	const std::string& TextureSystem::RegisteredTexturesContains(Ref<Texture> texture)
	{
		for (auto& entry : m_SystemState.RegisteredTextures)
		{
			if (*entry.second.get() == *texture.get())
				return entry.first;
		}
		return 0;
	}

}
