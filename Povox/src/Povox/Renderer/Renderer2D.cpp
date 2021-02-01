#include "pxpch.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	struct QuadVertex {
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
	};

	struct Render2DData
	{
		uint32_t MaxQuads = 10000;
		uint32_t MaxVertices = MaxQuads * 4;
		uint32_t MaxIndices = MaxQuads * 6;

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Texture> WhiteTexture;

		uint32_t QuadIndexCount = 0;

		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;
	};

	static Render2DData s_QuadData;

	void Renderer2D::Init()
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.QuadVertexArray = VertexArray::Create();

		s_QuadData.QuadVertexBuffer = VertexBuffer::Create(s_QuadData.MaxQuads * sizeof(QuadVertex));
		s_QuadData.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" }
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

		s_QuadData.TextureShader = Shader::Create("assets/shaders/Texture.glsl");

		s_QuadData.TextureShader->Bind();
		s_QuadData.TextureShader->SetInt("u_Texture", 0);
	}

	void Renderer2D::Shutdown()
	{
		PX_PROFILE_FUNCTION();


	}

	void Renderer2D::BeginScene(OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.TextureShader->Bind();
		s_QuadData.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());


		s_QuadData.QuadVertexBufferPtr = s_QuadData.QuadVertexBufferBase;
	}

	void Renderer2D::Flush()
	{
		RenderCommand::DrawIndexed(s_QuadData.QuadVertexArray, s_QuadData.QuadIndexCount);
	}

	void Renderer2D::EndScene()
	{
		PX_PROFILE_FUNCTION();

		uint32_t dataSize = (uint8_t)s_QuadData.QuadVertexBufferPtr - (uint8_t)s_QuadData.QuadVertexBufferBase;
		s_QuadData.QuadVertexBuffer->SetData(s_QuadData.QuadVertexBufferBase, dataSize);

		Flush();
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.QuadVertexBufferPtr->Position = position;
		s_QuadData.QuadVertexBufferPtr->Color = color;
		s_QuadData.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_QuadData.QuadVertexBufferPtr++;

		s_QuadData.QuadVertexBufferPtr->Position = { position.x + size.x, position.y, 0.0f };
		s_QuadData.QuadVertexBufferPtr->Color = color;
		s_QuadData.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_QuadData.QuadVertexBufferPtr++;

		s_QuadData.QuadVertexBufferPtr->Position = { position.x + size.x, position.y + size.y, 0.0f };
		s_QuadData.QuadVertexBufferPtr->Color = color;
		s_QuadData.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_QuadData.QuadVertexBufferPtr++;

		s_QuadData.QuadVertexBufferPtr->Position = { position.x, position.y + size.y, 0.0f };
		s_QuadData.QuadVertexBufferPtr->Color = color;
		s_QuadData.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_QuadData.QuadVertexBufferPtr++;

		s_QuadData.QuadIndexCount += 6;

		/*s_QuadData.TextureShader->SetFloat("u_TilingFactor", 1.0f);
		s_QuadData.WhiteTexture->Bind();
		
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData.TextureShader->SetMat4("u_Transform", transform);

		s_QuadData.QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData.QuadVertexArray);
		*/
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintingColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.TextureShader->SetFloat4("u_Color", tintingColor);
		s_QuadData.TextureShader->SetFloat("u_TilingFactor", tilingFactor);
		texture->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData.TextureShader->SetMat4("u_Transform", transform);
		
		s_QuadData.QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData.QuadVertexArray);
	}

	//Rotatables

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, float rotation, const glm::vec2 size, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, rotation, size, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, float rotation, const glm::vec2 size, const glm::vec4& color)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.TextureShader->SetFloat4("u_Color", color);
		s_QuadData.WhiteTexture->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData.TextureShader->SetMat4("u_Transform", transform);

		s_QuadData.QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData.QuadVertexArray);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, float rotation, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, rotation, size, texture, tilingFactor, tintingColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, float rotation, const glm::vec2 size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintingColor)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData.TextureShader->SetFloat4("u_Color", tintingColor);
		s_QuadData.TextureShader->SetFloat("u_TilingFactor", tilingFactor);
		texture->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData.TextureShader->SetMat4("u_Transform", transform);

		s_QuadData.QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData.QuadVertexArray);
	}

}