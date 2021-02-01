#pragma once
#include "Povox/Renderer/OrthographicCamera.h"

#include "Povox/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Povox {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(OrthographicCamera& camera); // TODO: Needs to be filled with environment (e.g. lighting), camera...
		static void EndScene();

		static void Flush();

	// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));

		static void DrawRotatedQuad(const glm::vec2& position, float rotation, const glm::vec2 size, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, float rotation, const glm::vec2 size, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, float rotation, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4 & tintingColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, float rotation, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4 & tintingColor = glm::vec4(1.0f));
	};

}
