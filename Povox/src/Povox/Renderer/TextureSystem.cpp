#include "pxpch.h"
#include "Povox/Renderer/TextureSystem.h"

#include "Povox/Core/Log.h"
#include "Povox/Renderer/Renderer.h"

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


		PX_CORE_INFO("TextureSystem::Init: Initializing...");

		m_SystemState.Config.TexturesPath = "assets/textures/";
		m_SystemState.RegisteredTextures.reserve(m_SystemState.Config.MaxTextures);

		CreateDefaultTexture();
		ResetFixedTextures(); //Also resets active textures

		PX_CORE_INFO("TextureSystem::Init: Initialization complete.");
	}


	void TextureSystem::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_INFO("TextureSystem::Shutdown: Starting...");
		
		ResetFixedTextures();
		// TODO: Clean all textures upon shutdown by calling destroy texture? Or let smart pointers handle the cleanup -> clear the pixel array(s)
		for (auto& texture : m_SystemState.RegisteredTextures)
		{
			texture.second->Free();
		}
		m_SystemState.RegisteredTextures.clear();
		std::memset(m_SystemState.ActiveTextures.data(), 0, m_SystemState.ActiveTextures.size());
		std::memset(m_SystemState.ActiveTexturesCounter.data(), 0, m_SystemState.ActiveTexturesCounter.size());

		PX_CORE_INFO("TextureSystem::Shutdown: Completed.");
	}

	// TODO: Alternatively, use the whole path here instead of name in case textures exist in multiple files
	Ref<Texture> TextureSystem::RegisterTexture(const std::string& name)
	{
		PX_PROFILE_FUNCTION();


		Ref<Texture> texture = RegisteredTexturesContains(name);
		if (texture)
			return texture;


		// TODO: query the ending from texture format/type | Check if this returns null if path is wrong
		texture = Texture2D::Create(m_SystemState.Config.TexturesPath + name + ".png", name);
		if (!texture)
		{
			return m_SystemState.DefaultTexture != nullptr ? m_SystemState.DefaultTexture : nullptr;
		}
		m_SystemState.RegisteredTextures[name] = texture;

		return texture;
	}

	Ref<Texture> TextureSystem::RegisterTexture(const std::string& name, Ref<Texture> newTexture)
	{
		PX_PROFILE_FUNCTION();


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

	/**
	 * Gets the texture from the registered textures array of this TextureSystem. Returns nullptr and error message not able to register.
	 */
	Ref<Texture> TextureSystem::GetTexture(const std::string& name)
	{
		PX_PROFILE_FUNCTION();


		if (RegisteredTexturesContains(name))
			return m_SystemState.RegisteredTextures[name];

		PX_CORE_WARN("TextureSystem::GetTexture: Texture '{0}' not found. Returning nullptr!", name);
		return nullptr;
	}
	/**
	 * Gets the texture from the registered textures array of this TextureSystem or tries to Register it using 'name' Returns nullptr and error message not able to register.
	 */
	Ref<Texture> TextureSystem::GetOrRegisterTexture(const std::string& name)
	{
		PX_PROFILE_FUNCTION();


		if (Ref<Texture> texture = GetTexture(name))
			return texture;

		PX_CORE_WARN("TextureSystem::GetOrRegisterTexture: Attempting to register from '{1}{0}.png'", name, m_SystemState.Config.TexturesPath);
		return RegisterTexture(name);
	}

	/**
	 * The texture is not needed to be in the RegisteredTexture Array!
	 */
	const uint32_t TextureSystem::BindTexture(Ref<Texture> texture)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_ASSERT(texture, "No texture set");
		for (uint32_t i = 0; i < m_SystemState.NextTextureSlot; i++)
		{
			if (*texture.get() == *m_SystemState.ActiveTextures[i].get())
			{
				m_SystemState.ActiveTexturesCounter[i]++;
				return i;
			}
		}

		if (m_SystemState.NextTextureSlot + 1 == MAX_TEXTURE_SLOTS)
		{
			PX_CORE_WARN("TextureSystem::BindTexture: Max_Texture_Slots reached. Pls draw and reset!");
			return MAX_TEXTURE_SLOTS + 1;
		}

		m_SystemState.ActiveTextures[m_SystemState.NextTextureSlot] = texture;
		m_SystemState.ActiveTexturesCounter[m_SystemState.NextTextureSlot] = 1;


		return m_SystemState.NextTextureSlot++;
	}
	const uint32_t TextureSystem::BindTexture(const std::string& name)
	{
		Ref<Texture> texture = RegisteredTexturesContains(name);
		return BindTexture(texture);
	}

	/**
	 * When ActiveTextures gets reset, all textures up until this slot dont get touched
	 * This Binding also overrides the first non-fixed texture currently bound
	 */
	const uint32_t TextureSystem::BindFixedTexture(Ref<Texture> texture)
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_ASSERT(texture, "No texture set");
		if(RegisteredTexturesContains(texture).empty())
		{
			PX_CORE_WARN("Texture {} not registered!", texture);
		}

		PX_CORE_ASSERT(m_SystemState.NextFixedSlot < m_SystemState.Config.MaxTextureSlots, "No more Textures can be bound into ActiveTextures!");
		m_SystemState.ActiveTextures[m_SystemState.NextFixedSlot] = texture;
		m_SystemState.ActiveTexturesCounter[m_SystemState.NextFixedSlot] = 0;

		m_SystemState.NextFixedSlot++;
		m_SystemState.NextTextureSlot = m_SystemState.NextFixedSlot;

		return m_SystemState.NextFixedSlot - 1;
	}
	const uint32_t TextureSystem::BindFixedTexture(const std::string& name)
	{
		Ref<Texture> texture = RegisteredTexturesContains(name);
		return BindFixedTexture(texture);
	}


	/*
	* This only resets the texture slots that are not persistently bound, but resets all active textures counters to 0
	*/
	void TextureSystem::ResetActiveTextures()
	{
		PX_PROFILE_FUNCTION();


		if (!m_SystemState.DefaultTexture)
			PX_CORE_WARN("TextureSystem::ResetActiveTextures: No DefaultTexture has been set!");

		for (uint32_t i = m_SystemState.NextFixedSlot; i < m_SystemState.Config.MaxTextureSlots; i++)
		{
			m_SystemState.ActiveTextures[i] = m_SystemState.DefaultTexture;
			m_SystemState.ActiveTexturesCounter[i] = 0;
		}
		for (uint32_t i = 0; i < m_SystemState.Config.MaxTextureSlots; i++)
		{
			m_SystemState.ActiveTexturesCounter[i] = 0;
		}
		m_SystemState.NextTextureSlot = m_SystemState.NextFixedSlot;
	}

	/**
	 * Resets the FixedTextures -> Active Textures also get reset, all pointers get set to 0
	 */
	void TextureSystem::ResetFixedTextures()
	{
		m_SystemState.NextFixedSlot = 0;
		ResetActiveTextures();
	}

	const std::array<Ref<Texture>, MAX_TEXTURE_SLOTS>& TextureSystem::GetActiveTextures()
	{
		PX_PROFILE_FUNCTION();


		for (uint32_t i = m_SystemState.NextTextureSlot; i < MAX_TEXTURE_SLOTS; i++)
		{
			m_SystemState.ActiveTextures[i] = m_SystemState.DefaultTexture;
		}

		return m_SystemState.ActiveTextures;
	}

	void TextureSystem::CreateDefaultTexture()
	{
		m_SystemState.DefaultTexture = RegisterTexture("DefaultPXTexture");
		PX_CORE_ASSERT(m_SystemState.DefaultTexture, "No Default Texture was set!");
	}

	//TODO: maybe return boolean instead and pass in pointers to texture and name
	Ref<Texture> TextureSystem::RegisteredTexturesContains(const std::string& name)
	{
		if (m_SystemState.RegisteredTextures.find(name) == m_SystemState.RegisteredTextures.end())
			return nullptr;
		return m_SystemState.RegisteredTextures[name];
	}

	//TODO: maybe return boolean instead and pass in pointers to texture and name
	const std::string& TextureSystem::RegisteredTexturesContains(Ref<Texture> texture)
	{
		for (auto& entry : m_SystemState.RegisteredTextures)
		{
			if (*entry.second.get() == *texture.get())
				return entry.first;
		}
		return std::string();
	}

}
