#pragma once
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Texture.h"
#include "Povox/Renderer/Shader.h"

namespace Povox {


	class Material
	{
	public:
		~Material() = default;

		virtual const uint64_t GetRendererID() const = 0;

		static Ref<Material> Create(Ref<Shader> shader, const std::string& name = "MaterialName");

		virtual uint32_t BindTexture(const std::string& name) = 0;
		//virtual std::array<Ref<Texture>, 32> GetActiveTextures() = 0;
		virtual Ref<Shader> GetShader() = 0;
		virtual const Ref<Shader> GetShader() const = 0;

		virtual bool operator==(const Texture& other) const = 0;
	};


}
