#include "pxpch.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Povox {

	struct Render2DStorage
	{
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> FlatColorShader;
		Ref<Shader> TextureShader;
	};

	static Render2DStorage* s_QuadData;

	void Renderer2D::Init()
	{
		s_QuadData = new Render2DStorage();
		s_QuadData->QuadVertexArray = Povox::VertexArray::Create();

		float squareVertices[4 * 5] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};
		Ref<VertexBuffer> squareVB = VertexBuffer::Create(squareVertices, sizeof(squareVertices));
		squareVB->SetLayout({
			{ Povox::ShaderDataType::Float3, "a_Position" },
			{ Povox::ShaderDataType::Float2, "a_TexCoord" }
			});
		s_QuadData->QuadVertexArray->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
		Ref<IndexBuffer> squareIB = IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t));
		s_QuadData->QuadVertexArray->SetIndexBuffer(squareIB);

		s_QuadData->FlatColorShader = Shader::Create("assets/shaders/FlatColor.glsl");
		s_QuadData->TextureShader = Shader::Create("assets/shaders/Texture.glsl");

		s_QuadData->TextureShader->Bind();
		s_QuadData->TextureShader->SetInt("u_Texture", 0);
	}

	void Renderer2D::Shutdown()
	{
		delete s_QuadData;
	}

	void Renderer2D::BeginScene(OrthographicCamera& camera)
	{
		s_QuadData->FlatColorShader->Bind();
		s_QuadData->FlatColorShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

		s_QuadData->TextureShader->Bind();
		s_QuadData->TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
	}

	void Renderer2D::EndScene()
	{
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const glm::vec4& color)
	{
		s_QuadData->FlatColorShader->Bind();
		s_QuadData->FlatColorShader->SetFloat4("u_Color", color);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData->FlatColorShader->SetMat4("u_Transform", transform);

		s_QuadData->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData->QuadVertexArray);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2 size, const Ref<Texture2D>& texture)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture);

	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2 size, const Ref<Texture2D>& texture)
	{
		s_QuadData->TextureShader->Bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_QuadData->TextureShader->SetMat4("u_Transform", transform);

		texture->Bind();

		s_QuadData->QuadVertexArray->Bind();
		RenderCommand::DrawIndexed(s_QuadData->QuadVertexArray);
	}

}