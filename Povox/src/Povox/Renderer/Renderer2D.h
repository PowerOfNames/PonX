#pragma once
#include "Povox/Renderer/OrthographicCamera.h"
#include "Povox/Renderer/Camera.h"
#include "Povox/Renderer/EditorCamera.h"

#include "Povox/Scene/Components.h"

#include "Povox/Renderer/Texture.h"
#include "Povox/Renderer/SubTexture2D.h"

#include <glm/glm.hpp>

namespace Povox {

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const OrthographicCamera& camera); // TODO: Needs to be filled with environment (e.g. lighting), camera...
		static void BeginScene(const EditorCamera& camera); // TODO: Needs to be filled with environment (e.g. lighting), camera...
		static void EndScene();

		static void Flush();

	// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color);
		static void DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));

		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f), int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f), int entityID = -1);

		//Rotation in radians
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4 & tintingColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4 & tintingColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor = 1.0f, const glm::vec4& tintingColor = glm::vec4(1.0f));
	
		static void DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int enittyID);
		
		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};
	
		static Statistics GetStats();
		static void ResetStats();

	private:
		static void NextBatch();
		static void StartBatch();
	};

}
