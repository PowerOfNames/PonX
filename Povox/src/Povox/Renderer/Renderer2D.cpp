#include "pxpch.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/RenderCommand.h"
#include "Povox/Renderer/QuadVertexStruct.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Povox {


	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; //TODO: needs to be dynamic to the GPU, comes from Renderer Capabilities

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 is reserved for whiteTexture

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;
	};

	static Renderer2DData s_QuadData;

	void Renderer2D::Init()
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.QuadVertexArray = VertexArray::Create();

		s_QuadData.QuadVertexBuffer = VertexBuffer::Create(s_QuadData.MaxVertices * sizeof(QuadVertex));
		s_QuadData.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float, "a_TexID" },
			{ ShaderDataType::Float, "a_TilingFactor" }
			});
		s_QuadData.QuadVertexArray->AddVertexBuffer(s_QuadData.QuadVertexBuffer);

		s_QuadData.QuadVertexBufferBase = new QuadVertex[s_QuadData.MaxVertices];

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

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_QuadData.MaxIndices);
		s_QuadData.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		s_QuadData.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_QuadData.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));


		int32_t samplers[s_QuadData.MaxTextureSlots];
		for (uint32_t i = 0; i < s_QuadData.MaxTextureSlots; i++)
			samplers[i] = i;

		s_QuadData.TextureShader = Shader::Create("assets/shaders/Texture.glsl");
		s_QuadData.TextureShader->Bind();
		s_QuadData.TextureShader->SetIntArray("u_Textures", samplers, s_QuadData.MaxTextureSlots);

		s_QuadData.TextureSlots[0] = s_QuadData.WhiteTexture;

		s_QuadData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_QuadData.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_QuadData.QuadVertexPositions[2] = { 0.5f, 0.5f, 0.0f, 1.0f };
		s_QuadData.QuadVertexPositions[3] = { -0.5f, 0.5f, 0.0f, 1.0f };
	}

	void Renderer2D::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		delete[] s_QuadData.QuadVertexBufferBase;
	}

	void Renderer2D::BeginScene(OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.TextureShader->Bind();
		s_QuadData.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_QuadData.QuadIndexCount = 0;
		s_QuadData.QuadVertexBufferPtr = s_QuadData.QuadVertexBufferBase;

		s_QuadData.TextureSlotIndex = 1;
	}

	void Renderer2D::Flush()
	{
		for (uint32_t i = 0; i < s_QuadData.TextureSlotIndex; i++)
			s_QuadData.TextureSlots[i]->Bind(i);

		RenderCommand::DrawIndexed(s_QuadData.QuadVertexArray, s_QuadData.QuadIndexCount);
		s_QuadData.Stats.DrawCalls++;
	}

	void Renderer2D::EndScene()
	{
		PX_PROFILE_FUNCTION();

		uint32_t dataSize = (uint32_t)((uint8_t*)s_QuadData.QuadVertexBufferPtr - (uint8_t*)s_QuadData.QuadVertexBufferBase);
		s_QuadData.QuadVertexBuffer->SetData(s_QuadData.QuadVertexBufferBase, dataSize);

		Flush();
	}

	void Renderer2D::FlushAndReset()
	{
		EndScene();

		s_QuadData.QuadIndexCount = 0;
		s_QuadData.QuadVertexBufferPtr = s_QuadData.QuadVertexBufferBase;

		s_QuadData.TextureSlotIndex = 1;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color)
	{
		PX_PROFILE_FUNCTION();


		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		constexpr float whiteTextureID = 0.0f;
		constexpr float tilingFactor = 1.0f;
		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = color;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = whiteTextureID;
			s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;

		s_QuadData.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintingColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		PX_PROFILE_FUNCTION();


		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
		
		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_QuadData.TextureSlotIndex; i++)
		{
			if (*s_QuadData.TextureSlots[i].get() == *texture.get()) // s_QuadData.TextureSlots[i] == texture -> compares the shared ptr, so .get() gives the ptr and * dereferences is to the fnc uses the boolean == operator defined in the openGLTexture2D class
			{
				textureIndex = (float)i;
				break;
			}

		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_QuadData.TextureSlotIndex;
			s_QuadData.TextureSlots[s_QuadData.TextureSlotIndex] = texture;
			s_QuadData.TextureSlotIndex++;
		}

		
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = tintingColor;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = textureIndex;
			s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;

		s_QuadData.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, subTexture, tilingFactor, tintingColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		PX_PROFILE_FUNCTION();


		const glm::vec2* textureCoords = subTexture->GetTexCoords();
		const Ref<Texture2D> texture = subTexture->GetTexture();

		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_QuadData.TextureSlotIndex; i++)
		{
			if (*s_QuadData.TextureSlots[i].get() == *texture.get()) // s_QuadData.TextureSlots[i] == texture -> compares the shared ptr, so .get() gives the ptr and * dereferences is to the fnc uses the boolean == operator defined in the openGLTexture2D class
			{
				textureIndex = (float)i;
				break;
			}

		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_QuadData.TextureSlotIndex;
			s_QuadData.TextureSlots[s_QuadData.TextureSlotIndex] = texture;
			s_QuadData.TextureSlotIndex++;
		}


		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = tintingColor;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = textureIndex;
			s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;

		s_QuadData.Stats.QuadCount++;
	}

	//Rotatables

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const glm::vec4& color)
	{
		PX_PROFILE_FUNCTION();


		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		constexpr float whiteTextureID = 0.0f;
		constexpr float tilingFactor = 1.0f;
		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = color;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = whiteTextureID;
			s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;

		s_QuadData.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintingColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		PX_PROFILE_FUNCTION();


		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_QuadData.TextureSlotIndex; i++)
		{
			if (*s_QuadData.TextureSlots[i].get() == *texture.get()) // s_QuadData.TextureSlots[i] == texture -> compares the shared ptr, so .get() gives the ptr and * dereferences is to the fnc uses the boolean == operator defined in the openGLTexture2D class
			{
				textureIndex = (float)i;
				break;
			}

		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_QuadData.TextureSlotIndex;
			s_QuadData.TextureSlots[s_QuadData.TextureSlotIndex] = texture;
			s_QuadData.TextureSlotIndex++;
		}

		constexpr glm::vec2 textureCoords[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = tintingColor;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = textureIndex;
			s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;

		s_QuadData.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, subTexture, tilingFactor, tintingColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2 size, float rotation, const Ref<SubTexture2D>& subTexture, float tilingFactor, const glm::vec4& tintingColor)
	{
		PX_PROFILE_FUNCTION();


		const glm::vec2* textureCoords = subTexture->GetTexCoords();
		const Ref<Texture2D> texture = subTexture->GetTexture();

		if (s_QuadData.QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_QuadData.TextureSlotIndex; i++)
		{
			if (*s_QuadData.TextureSlots[i].get() == *texture.get()) // s_QuadData.TextureSlots[i] == texture -> compares the shared ptr, so .get() gives the ptr and * dereferences is to the fnc uses the boolean == operator defined in the openGLTexture2D class
			{
				textureIndex = (float)i;
				break;
			}

		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_QuadData.TextureSlotIndex;
			s_QuadData.TextureSlots[s_QuadData.TextureSlotIndex] = texture;
			s_QuadData.TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		for (uint32_t i = 0; i < 4; i++)
		{
			s_QuadData.QuadVertexBufferPtr->Position = transform * s_QuadData.QuadVertexPositions[i];
			s_QuadData.QuadVertexBufferPtr->Color = tintingColor;
			s_QuadData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_QuadData.QuadVertexBufferPtr->TexID = textureIndex;
			s_QuadData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_QuadData.QuadVertexBufferPtr++;
		}
		s_QuadData.QuadIndexCount += 6;

		s_QuadData.Stats.QuadCount++;
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