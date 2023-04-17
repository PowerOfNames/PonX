#include "pxpch.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Renderer/Renderable.h"
#include "Povox/Renderer/Renderer.h"

#include "Povox/Renderer/Buffer.h"
#include "Povox/Renderer/Material.h"
#include "Povox/Renderer/Shader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 60000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; //TODO: needs to be dynamic to the GPU, comes from Renderer Capabilities

		// Common
		Ref<Texture2D> WhiteTexture;
		uint32_t WhiteTextureSlot;

		// Quads
		std::vector<Ref<Buffer>> QuadVertexBuffers;
		std::vector<QuadVertex*> QuadVertexBufferBases;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::vector<Ref<Buffer>> QuadIndexBuffers;
		uint32_t QuadIndexCount = 0;

		Ref<Material> QuadMaterial;
		glm::vec4 QuadVertexPositions[4];

		// Lines

		CameraUniform CameraData;


		Renderer2D::Statistics Stats;

		std::vector<Renderable> RenderedObjects;
	};

	static Renderer2DData s_QuadData;

	void Renderer2D::Init()
	{
		PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("Renderer2D::Init: Starting...");
		// TODO: Instead of taking a name and a path, just take in a name and pass the path upon ShaderLib creation inside the RendererBackend, pointing to root/.../assets/shaders/
		Renderer::GetShaderLibrary()->Add("TextureShader", Shader::Create("assets/shaders/Texture.glsl"));
		Renderer::GetShaderLibrary()->Add("FlatColorShader", Shader::Create("assets/shaders/FlatColor.glsl"));
		Renderer::GetShaderLibrary()->Add("Renderer2D_Quad", Shader::Create("assets/shaders/Renderer2D_Quad.glsl"));
		


		BufferSpecification vertexBufferSpecs{};
		vertexBufferSpecs.Usage = BufferUsage::VERTEX_BUFFER;
		vertexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		vertexBufferSpecs.ElementCount = s_QuadData.MaxVertices;
		vertexBufferSpecs.ElementSize = sizeof(QuadVertex);
		vertexBufferSpecs.Size = sizeof(QuadVertex) * s_QuadData.MaxVertices;

		s_QuadData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_QuadData.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_QuadData.QuadVertexPositions[2] = { 0.5f, 0.5f, 0.0f, 1.0f };
		s_QuadData.QuadVertexPositions[3] = { -0.5f, 0.5f, 0.0f, 1.0f };

		uint32_t* quadIndices = new uint32_t[s_QuadData.MaxIndices];
		uint32_t offset = 0;
		for (uint32_t index = 0; index < s_QuadData.MaxIndices; index += 6)
		{
			quadIndices[index + 0] = offset + 0;
			quadIndices[index + 1] = offset + 1;
			quadIndices[index + 2] = offset + 2;

			quadIndices[index + 3] = offset + 2;
			quadIndices[index + 4] = offset + 3;
			quadIndices[index + 5] = offset + 0;

			offset += 4;
		}
		BufferSpecification indexBufferSpecs{};
		indexBufferSpecs.Usage = BufferUsage::INDEX_BUFFER;
		indexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		indexBufferSpecs.ElementCount = s_QuadData.MaxIndices;
		indexBufferSpecs.ElementSize = sizeof(uint32_t);
		indexBufferSpecs.Size = sizeof(uint32_t) * s_QuadData.MaxIndices;
		indexBufferSpecs.Data = quadIndices;

		uint32_t maxFrames = Renderer::GetSpecification().MaxFramesInFlight;
		s_QuadData.QuadVertexBuffers.resize(maxFrames);
		s_QuadData.QuadVertexBufferBases.resize(maxFrames);

		s_QuadData.QuadIndexBuffers.resize(maxFrames);
		for (uint32_t i = 0; i < maxFrames; i++)
		{
			s_QuadData.QuadVertexBuffers[i] = Buffer::Create(vertexBufferSpecs);
			s_QuadData.QuadVertexBufferBases[i] = new QuadVertex[s_QuadData.MaxVertices];

			s_QuadData.QuadIndexBuffers[i] = Buffer::Create(indexBufferSpecs);
		}
		delete[] quadIndices;
		s_QuadData.QuadMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("Renderer2D_Quad"), "Quad");

		/* -> move to Pipeline.Layout to check against Pipeline.Shader.Layout -> For VertexInput checkup
		s_QuadData.QuadVertexBuffer->SetLayout({
			{ ShaderUtils::ShaderDataType::Float3, "a_Position" },
			{ ShaderUtils::ShaderDataType::Float4, "a_Color" },
			{ ShaderUtils::ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderUtils::ShaderDataType::Int, "a_EntityID" }
			});
		*/

		s_QuadData.WhiteTexture = Texture2D::Create(1, 1, 4, "WhiteTexture");
		uint32_t whiteTextureData = 0xffffffff;
		s_QuadData.WhiteTexture->SetData(&whiteTextureData);
		Renderer::GetTextureSystem()->RegisterTexture("WhiteTexture", s_QuadData.WhiteTexture);

		s_QuadData.WhiteTextureSlot = Renderer::GetTextureSystem()->BindFixedTexture(s_QuadData.WhiteTexture);
		

		PX_CORE_TRACE("Renderer2D::Init: Completed.");
	}

	void Renderer2D::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		//TODO: Investigate shutdown read access error here!
		//delete[] s_QuadData.QuadVertexBufferBase;
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();

		s_QuadData.CameraData.ViewProjMatrix = camera.GetViewProjectionMatrix();
		StartBatch();
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		PX_PROFILE_FUNCTION();

		s_QuadData.CameraData.ViewProjMatrix = camera.GetProjection() * glm::inverse(transform);
		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.CameraData.ViewMatrix = camera.GetViewMatrix();
		s_QuadData.CameraData.ProjectionMatrix = camera.GetProjection();
		s_QuadData.CameraData.ViewProjMatrix = camera.GetViewProjectionMatrix();
		Renderer::UpdateCamera(s_QuadData.CameraData);
		StartBatch();
	}

	void Renderer2D::StartBatch()
	{
		PX_PROFILE_FUNCTION();


		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		s_QuadData.QuadIndexCount = 0;
		s_QuadData.QuadVertexBufferPtr = s_QuadData.QuadVertexBufferBases[currentFrame];

		Renderer::GetTextureSystem()->ResetActiveTextures();
		s_QuadData.RenderedObjects.clear();
	}

	void Renderer2D::Flush()
	{
		PX_PROFILE_FUNCTION();


		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		if (s_QuadData.QuadIndexCount == 0)
			return; // nothing to draw

		uint32_t dataSize = (uint32_t)((uint8_t*)s_QuadData.QuadVertexBufferPtr - (uint8_t*)s_QuadData.QuadVertexBufferBases[currentFrame]);
		s_QuadData.QuadVertexBuffers[currentFrame]->SetData(s_QuadData.QuadVertexBufferBases[currentFrame], dataSize);		
		Renderer::Draw(s_QuadData.QuadVertexBuffers[currentFrame], s_QuadData.QuadMaterial, s_QuadData.QuadIndexBuffers[currentFrame], s_QuadData.QuadIndexCount);

		
		s_QuadData.Stats.DrawCalls++;
	}

	void Renderer2D::EndScene()
	{
		PX_PROFILE_FUNCTION();


		Flush();
		Renderer::GetPipelineStats(s_QuadData.Stats.PipelineStatNames, s_QuadData.Stats.PipelineStats);
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}


// Quads ColorOnly
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color)
	{
		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });		

		DrawQuad(transform, color);
	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, UUID entityID)
	{
		//PX_PROFILE_FUNCTION();


		constexpr glm::vec2 textureCoords[4] = { {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f} };

		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = color;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = s_QuadData.WhiteTextureSlot;
			s_QuadData.QuadVertexBufferPtr->EntityID = entityID;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;
		s_QuadData.Stats.QuadCount++;
	}

// Quads Texture
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor, UUID entityID)
	{
		PX_PROFILE_FUNCTION();


		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
		
		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();
		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		//Will return the first slot possible if allSlotsFull is true
		uint32_t textureIndex = Renderer::GetTextureSystem()->BindTexture(texture);
		if (textureIndex >= s_QuadData.MaxTextureSlots)
			NextBatch();
		textureIndex = Renderer::GetTextureSystem()->BindTexture(texture);

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = tintingColor;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = textureIndex;
			s_QuadData.QuadVertexBufferPtr->EntityID = entityID;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;
		s_QuadData.Stats.QuadCount++;
	}

// Quads Subtexture
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, subTexture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, subTexture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor, UUID entityID)
	{
		const glm::vec2* textureCoords = subTexture->GetTexCoords();
		const Ref<Texture2D> texture = subTexture->GetTexture();

		uint32_t currentFrame = Renderer::GetCurrentFrameIndex();

		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		//float textureIndex = 0.0f;
		//for (uint32_t i = 1; i < s_QuadData.TextureSlotIndex; i++)
		//{
		//	if (*s_QuadData.TextureSlots[i].get() == *texture.get()) // s_QuadData.TextureSlots[i] == texture -> compares the shared ptr, so .get() gives the ptr and * dereferences is to the fnc uses the boolean == operator defined in the openGLTexture2D class
		//	{
		//		textureIndex = (float)i;
		//		break;
		//	}

		//}

		//if (textureIndex == 0.0f)
		//{
		//	textureIndex = (float)s_QuadData.TextureSlotIndex;
		//	s_QuadData.TextureSlots[s_QuadData.TextureSlotIndex] = texture;
		//	s_QuadData.TextureSlotIndex++;
		//}

		//for (uint32_t i = 0; i < 4; i++)
		//{
		//	s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
		//	s_QuadData.QuadVertexBufferPtr->Color = tintingColor;
		//	s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
		//	s_QuadData.QuadVertexBufferPtr->TexID = textureIndex;
		//	s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		//	s_QuadData.QuadVertexBufferPtr->EntityID = entityID;
		//	s_QuadData.QuadVertexBufferPtr++;
		//}
		//s_QuadData.QuadIndexCount += 6;

		//s_QuadData.Stats.QuadCount++;
	}

//Rotatables
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const glm::vec4& color)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
		
		DrawQuad(transform, texture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, subTexture, tilingFactor, tintingColor);
	}
	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, subTexture, tilingFactor, tintingColor);
	}


//Sprite
	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, UUID entityID)
	{
		DrawQuad(transform, src.Color, entityID);
	}




	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_QuadData.Stats;
	}
	void Renderer2D::ResetStats()
	{
		memset(&s_QuadData.Stats, 0, sizeof(Statistics));
	}
}
