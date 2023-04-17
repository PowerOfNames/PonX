#pragma once
#include "Povox/Core/Log.h"
#include "Povox/Core/Core.h"

#include "Povox/Renderer/Material.h"
#include "Povox/Renderer/RendererUID.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/Texture.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDescriptor.h"


namespace Povox {



	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(Ref<Shader> shader, const std::string& name);
		~VulkanMaterial() = default;

		virtual const uint64_t GetRendererID() const override { return m_RUID; }

		virtual uint32_t BindTexture(const std::string& name) override;
		//virtual std::array<Ref<Texture>, 32> GetActiveTextures() override;

		virtual Ref<Shader> GetShader() override { return m_Shader; }
		virtual const Ref<Shader> GetShader() const override { return m_Shader; }

		virtual bool operator==(const Texture& other) const override { return m_RUID == ((VulkanMaterial&)other).m_RUID; }
	private:
		RendererUID m_RUID;
		Ref<Shader> m_Shader = nullptr;
		std::string m_Name;
	};


}
