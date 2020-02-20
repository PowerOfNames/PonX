#include "pxpch.h"
#include "Povox/Renderer/VoxelRenderer.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/Texture.h"
#include "Povox/Renderer/RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>

#define VRENDERER_MAX_CUBE_COUNT 1000
#define VRENDERER_MAX_VERTEX_COUNT VRENDERER_MAX_CUBE_COUNT * 8
#define VRENDERER_MAX_INDEX_COUNT VRENDERER_MAX_CUBE_COUNT * 6 * 6



namespace Povox {

	struct VoxelRendererStorage
	{		
		Ref<VertexArray> VoxelVertexArray;
		Ref<Shader> VoxelShader;
		Ref<Texture> Textures[2];
		Vertex* DataBuffer;
		uint32_t IndexCount = 0;
		std::array<Vertex, 1000> Vertices;
	};


	static Vertex* CreateCube(Vertex* target, const glm::vec3& pos, float scale, float texID)
	{
		target->Position = { pos.x, pos.y, pos.z};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 0.0f, 0.0f };
		target->TexID = texID;
		target++;

		target->Position = { pos.x + scale,  pos.y, pos.z};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 1.0f, 0.0f };
		target->TexID = texID;
		target++;

		target->Position = { pos.x + scale, pos.y, pos.z + scale};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 1.0f, 1.0f };
		target->TexID = texID;
		target++;

		target->Position = { pos.x, pos.y, pos.z + scale};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 0.0f, 1.0f };
		target->TexID = texID;
		target++;

		target->Position = { pos.x, pos.y + scale, pos.z};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 0.0f, 0.0f };
		target->TexID = texID;
		target++;


		target->Position = { pos.x + scale, pos.y + scale, pos.z};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 1.0f, 0.0f };
		target->TexID = texID;
		target++;

		target->Position = { pos.x + scale, pos.y + scale, pos.z + scale};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 1.0f, 1.0f };
		target->TexID = texID;
		target++;

		target->Position = { pos.x, pos.y + scale, pos.z + scale};
		target->Color = { 32.0f / 255, 95.0f / 255, 83.0f / 255, 1.0f };
		target->TexCoord = { 0.0f, 1.0f };
		target->TexID = texID;
		target++;

		return target;
	}

	static VoxelRendererStorage* s_VoxelStorage;

	void VoxelRenderer::Init()
	{
		PX_PROFILE_FUNCTION();

		s_VoxelStorage = new VoxelRendererStorage();
		s_VoxelStorage->VoxelVertexArray = VertexArray::Create();
		
		s_VoxelStorage->DataBuffer = s_VoxelStorage->Vertices.data();

		s_VoxelStorage->DataBuffer = CreateCube(s_VoxelStorage->DataBuffer, {0.0f, 0.0f, 0.0f}, 1.0f, 0.0f);
		s_VoxelStorage->DataBuffer = CreateCube(s_VoxelStorage->DataBuffer, {3.0f, 3.0f, 0.0f}, 2.0f, 1.0f);

		//Create the Buffer
		Ref<VertexBuffer> cubeVB = VertexBuffer::Create(s_VoxelStorage->Vertices.data(), s_VoxelStorage->Vertices.size());	 // normal rendering data submission works fine
		//Ref<VertexBuffer> cubeVB = VertexBuffer::CreateBatch(VRENDERER_MAX_VERTEX_COUNT * sizeof(Vertex)); // <-- 1000 cubes
		cubeVB->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float, "a_TexID" }
		});
		//Set the layout
		s_VoxelStorage->VoxelVertexArray->AddVertexBuffer(cubeVB);
		/*
		std::array<uint32_t, VRENDERER_MAX_INDEX_COUNT> indices;
		uint32_t* indexbuffer = indices.data();

		uint32_t offset = 0;
		for (uint32_t i = 0; i < VRENDERER_MAX_INDEX_COUNT; i++)
		{
			//Front
			indexbuffer[i++] = offset + 0;
			indexbuffer[i++] = offset + 1;
			indexbuffer[i++] = offset + 5;
			indexbuffer[i++] = offset + 5;
			indexbuffer[i++] = offset + 4;
			indexbuffer[i++] = offset + 0;
			//Right
			indexbuffer[i++] = offset + 1;
			indexbuffer[i++] = offset + 2;
			indexbuffer[i++] = offset + 6;
			indexbuffer[i++] = offset + 6;
			indexbuffer[i++] = offset + 5;
			indexbuffer[i++] = offset + 1;
			//Back
			indexbuffer[i++] = offset + 2;
			indexbuffer[i++] = offset + 3;
			indexbuffer[i++] = offset + 7;
			indexbuffer[i++] = offset + 7;
			indexbuffer[i++] = offset + 6;
			indexbuffer[i++] = offset + 2;
			//Left
			indexbuffer[i++] = offset + 3;
			indexbuffer[i++] = offset + 0;
			indexbuffer[i++] = offset + 4;
			indexbuffer[i++] = offset + 4;
			indexbuffer[i++] = offset + 7;
			indexbuffer[i++] = offset + 3;
			//Top
			indexbuffer[i++] = offset + 4;
			indexbuffer[i++] = offset + 5;
			indexbuffer[i++] = offset + 6;
			indexbuffer[i++] = offset + 6;
			indexbuffer[i++] = offset + 7;
			indexbuffer[i++] = offset + 4;
			//Bottom
			indexbuffer[i++] = offset + 3;
			indexbuffer[i++] = offset + 2;
			indexbuffer[i++] = offset + 1;
			indexbuffer[i++] = offset + 1;
			indexbuffer[i++] = offset + 0;
			indexbuffer[i] = offset + 3;

			offset += 8;			
		}

		for (int i = 0; i < 200; i++)
		{
			std::cout << indices[i] << std::endl;
		}
		*/

		uint32_t indices[] =
		{
			0, 1, 5, 5, 4, 0,
			1, 2, 6, 6, 5, 1,
			2, 3, 7, 7, 6, 2,
			3, 0, 4, 4, 7, 3,
			4, 5, 6, 6, 7, 4,
			3, 2, 1, 1, 0, 3,

			 8,  9, 13, 13, 12,  8,
			 9, 10, 14, 14, 13,  9,
			10, 11, 15, 15, 14, 10,
			11,  8, 12, 12, 15, 11,
			12, 13, 14, 14, 15, 12,
			11, 10,  9,  9,  8, 11
		};

		Ref<IndexBuffer> cubeIB = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		s_VoxelStorage->VoxelVertexArray->SetIndexBuffer(cubeIB);

		s_VoxelStorage->Textures[0] = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_VoxelStorage->Textures[0]->SetData(&whiteTextureData, sizeof(uint32_t));

		s_VoxelStorage->Textures[1] = Texture2D::Create(1, 1);
		uint32_t purpleTextureData = 0xff00ffff;
		s_VoxelStorage->Textures[1]->SetData(&purpleTextureData, sizeof(uint32_t));

		s_VoxelStorage->VoxelShader = Shader::Create("assets/shaders/CubeFlatColored.glsl");

		//s_VoxelData->VoxelShader->Bind();
		//s_VoxelData->Textures[0]->Bind(0);
		//s_VoxelData->Textures[1]->Bind(1);
		//s_VoxelData->VoxelShader->SetIntArray("u_Textures", 0, 1);

		//s_VoxelData->VoxelVertexArray->SubmitVertexData(vertices);						  // Batching does not work.
	}

	void VoxelRenderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();

		delete s_VoxelStorage;
	}

	void VoxelRenderer::BeginScene(PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();

		s_VoxelStorage->VoxelShader->Bind();
		s_VoxelStorage->VoxelShader->SetMat4("u_ViewProjection", camera.GetViewProjection());
	}

	void VoxelRenderer::EndScene()
	{
		PX_PROFILE_FUNCTION();


	}

	void VoxelRenderer::Flush()
	{
		
	}

	void VoxelRenderer::DrawCube(glm::vec3 position, float scale, glm::vec4 color)
	{
		PX_PROFILE_FUNCTION();
		

		CreateCube(s_VoxelStorage->DataBuffer, position, scale, 0.0f);

		s_VoxelStorage->VoxelVertexArray->Bind();
		s_VoxelStorage->VoxelShader->Bind();
		s_VoxelStorage->Textures[0]->Bind(0);
		s_VoxelStorage->Textures[1]->Bind(1);
		s_VoxelStorage->VoxelShader->SetIntArray("u_Textures", 0, 1);


		s_VoxelStorage->VoxelVertexArray->SubmitVertexData(s_VoxelStorage->Vertices.data());
		//after this I somehow need to reset the DataBuffer pointer...

		RenderCommand::DrawIndexed(s_VoxelStorage->VoxelVertexArray);
	}

	void VoxelRenderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::SetViewport(0, 0, width, height);
	}
}