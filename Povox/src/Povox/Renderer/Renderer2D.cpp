#include "pxpch.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Povox/Utility/Misc.h"

namespace Povox {

	struct Render2DStorage
	{
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> TextureShader;
		Texture2DLibrary Textures;
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> MapData;
		Ref<Texture2D> GreenTexture;
	};

	static Render2DStorage* s_QuadData;

	void Renderer2D::Init()
	{
		PX_PROFILE_FUNCTION();


		s_QuadData = new Render2DStorage();
		s_QuadData->QuadVertexArray = VertexArray::Create();

		float squareVertices[4 * 5 * 3] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f,

			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f,

			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};
		Ref<VertexBuffer> squareVB = VertexBuffer::Create(squareVertices, sizeof(squareVertices));
		squareVB->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" }
			});
		s_QuadData->QuadVertexArray->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6 * 3] = { 
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4,
			8, 9, 10, 10, 11, 8
		};
		Ref<IndexBuffer> squareIB = IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t));
		s_QuadData->QuadVertexArray->SetIndexBuffer(squareIB);

		s_QuadData->WhiteTexture = Texture2D::Create("WhiteTexture", 1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_QuadData->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
		s_QuadData->Textures.Add("WhiteTexture", s_QuadData->WhiteTexture);

		s_QuadData->MapData = Texture2D::Create("MapDataTest", 8, 8);
		uint32_t mapData[64];
		for (unsigned int i = 0; i < 64; i++)
		{
			mapData[i] = 0xffffffff;
		}
		mapData[0] = 0x00000000;
		mapData[63] = 0x00000000;
		s_QuadData->MapData->SetData(&mapData, sizeof(uint32_t) * 64);
		s_QuadData->Textures.Add(s_QuadData->MapData);

		s_QuadData->GreenTexture = Texture2D::Create("GreenTexture", "assets/textures/green.png");
		s_QuadData->Textures.Add(s_QuadData->GreenTexture);

		s_QuadData->TextureShader = Shader::Create("assets/shaders/Texture.glsl");

		s_QuadData->TextureShader->Bind();
		s_QuadData->TextureShader->SetInt("u_Texture", 0);
	}

	void Renderer2D::Shutdown()
	{
		PX_PROFILE_FUNCTION();


		delete s_QuadData;
	}

	void Renderer2D::BeginScene(OrthographicCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		s_QuadData->TextureShader->Bind();

		s_QuadData->TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
	}

	void Renderer2D::EndScene()
	{
		PX_PROFILE_FUNCTION();


	}

	/*Renders a quad in 2D without depth*/
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	/*Renders a quad with possible depth*/
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color)
	{
		s_QuadData->TextureShader->SetFloat4("u_Color", color);
		s_QuadData->Textures.Get("WhiteTexture")->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData->TextureShader->SetMat4("u_Transform", transform);

		s_QuadData->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData->QuadVertexArray);
	}


	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const std::string& filepath)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, filepath);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const std::string& filepath)
	{
		s_QuadData->TextureShader->SetFloat4("u_Color", glm::vec4(1.0f));
		s_QuadData->Textures.Load(filepath)->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData->TextureShader->SetMat4("u_Transform", transform);

		s_QuadData->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData->QuadVertexArray);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture)
	{
		s_QuadData->TextureShader->SetFloat4("u_Color", glm::vec4(1.0f));
		s_QuadData->Textures.Add(texture);
		s_QuadData->Textures.Get(texture->GetName())->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData->TextureShader->SetMat4("u_Transform", transform);
		
		s_QuadData->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData->QuadVertexArray);
	}

}