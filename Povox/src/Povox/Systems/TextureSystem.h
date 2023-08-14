#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Renderer/Texture.h"



namespace Povox {

#define MAX_TEXTURE_SLOTS 32
#define MAX_TEXTURES 1024

	struct TextureSystemConfig
	{
		static const uint32_t MaxTextureSlots = MAX_TEXTURE_SLOTS;
		static const uint32_t MaxTextures = MAX_TEXTURES;

		std::string TexturesPath = "";
	};

	struct TextureSystemState
	{
		TextureSystemConfig Config{};

		std::array<Ref<Texture>, MAX_TEXTURE_SLOTS> ActiveTextures;
		std::array<uint32_t, MAX_TEXTURE_SLOTS> ActiveTexturesCounter; // index equal the active textures vector, entry is counter
		uint32_t NextFixedSlot = 0;
		uint32_t NextTextureSlot = 0;

		std::unordered_map<std::string, Ref<Texture>> RegisteredTextures;

		Ref<Texture> DefaultTexture = nullptr;
	};

	class TextureSystem
	{
	public:
		TextureSystem();
		~TextureSystem();

		void Init();
		void Shutdown();

		Ref<Texture> RegisterTexture(const std::string& name);
		Ref<Texture> RegisterTexture(const std::string& name, Ref<Texture> texture);
		Ref<Texture> GetTexture(const std::string& name);
		Ref<Texture> GetOrRegisterTexture(const std::string& name);

		const uint32_t BindTexture(Ref<Texture> texture);
		const uint32_t BindTexture(const std::string& name);

		const uint32_t BindFixedTexture(Ref<Texture> texture);
		const uint32_t BindFixedTexture(const std::string& name);

		void ResetFixedTextures();
		void ResetActiveTextures();

		const std::array<Ref<Texture>, MAX_TEXTURE_SLOTS>& GetActiveTextures();

		inline const TextureSystemState& GetState() const { return m_SystemState; }
		inline const TextureSystemConfig& GetConfig() const { return m_SystemState.Config; }

	private:
		void CreateDefaultTexture();
		Ref<Texture> RegisteredTexturesContains(const std::string& name);
		const std::string& RegisteredTexturesContains(Ref<Texture> texture);

	private:
		TextureSystemState m_SystemState = {};
	};

}
