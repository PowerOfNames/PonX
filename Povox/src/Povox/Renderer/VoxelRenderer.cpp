#include "pxpch.h"
#include "Povox/Renderer/VoxelRenderer.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/Texture.h"
#include "Povox/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>


namespace Povox {

	struct VoxelRendererStorage
	{
		Ref<VertexArray> VoxelVertexArray;
		Ref<Shader> VoxelShader;
		Ref<Texture> WhiteTexture;
	};

	static VoxelRendererStorage* s_VoxelData;

	void VoxelRenderer::Init()
	{
		PX_PROFILE_FUNCTION();

		s_VoxelData = new VoxelRendererStorage();
		s_VoxelData->VoxelVertexArray = VertexArray::Create();

		float vertices[8 * 3] =
		{
			-0.5f, -0.5f,  0.5f,		//Bottom, Left, Front
			-0.5f,  0.5f,  0.5f,		//Bottom, Right, Front
			-0.5f,  0.5f, -0.5f,		//Bottom, Right, Back
			-0.5f, -0.5f, -0.5f,		//Bottom, Left, Back
			 0.5f, -0.5f,  0.5f,		//Top, Left, Front
			 0.5f,  0.5f,  0.5f,		//Top, Right, Front
			 0.5f,	0.5f, -0.5f,		//Top, Right, Back
			 0.5f, -0.5f, -0.5f,		//Top, Left, Back
		};
		Ref<VertexBuffer> cubeVB = VertexBuffer::Create(vertices, sizeof(vertices));
		cubeVB->SetLayout({
			{ ShaderDataType::Float3, "a_Position" }
		});
		s_VoxelData->VoxelVertexArray->AddVertexBuffer(cubeVB);

		uint32_t indices[6 * 6] =
		{
			0, 1, 5, 5, 4, 0,			//Front
			1, 2, 6, 6, 5, 1,			//Right
			2, 3, 7, 7, 6, 2,			//Back
			3, 0, 4, 4, 7, 3,			//Left
			4, 5, 6, 6, 7, 4,			//Top
			3, 2, 1, 1, 0, 3,			//Bottom
		};
		Ref<IndexBuffer> cubeIB = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		s_VoxelData->VoxelVertexArray->SetIndexBuffer(cubeIB);

		s_VoxelData->WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_VoxelData->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		s_VoxelData->VoxelShader = Shader::Create("assets/shaders/CubeTextureFlat.glsl");

		s_VoxelData->VoxelShader->Bind();
		s_VoxelData->VoxelShader->SetInt("u_Texture", 0);
	}

	void VoxelRenderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		delete s_VoxelData;
	}

	void VoxelRenderer::BeginScene(PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();

		s_VoxelData->VoxelShader->Bind();
		s_VoxelData->VoxelShader->SetMat4("u_ViewProjection", camera.GetViewProjection());
	}

	void VoxelRenderer::EndScene()
	{
		PX_PROFILE_FUNCTION();


	}

	void VoxelRenderer::DrawCube(glm::vec3 position, glm::vec3 size, glm::vec4 color)
	{
		PX_PROFILE_FUNCTION();

		s_VoxelData->VoxelShader->Bind();
		s_VoxelData->VoxelShader->SetFloat4("u_Color", color);
		s_VoxelData->WhiteTexture->Bind();


		glm::mat4 rotation =	glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f))
								* glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) 
								* glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f))
		* rotation;
		s_VoxelData->VoxelShader->SetMat4("u_Transform", transform);

		s_VoxelData->VoxelVertexArray->Bind();
		RenderCommand::DrawIndexed(s_VoxelData->VoxelVertexArray);
	}

}