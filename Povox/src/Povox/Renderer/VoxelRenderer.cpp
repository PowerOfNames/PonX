#include "pxpch.h"
#include "Povox/Renderer/VoxelRenderer.h"

#include "Povox/Renderer/VertexArray.h"
#include "Povox/Renderer/Shader.h"
#include "Povox/Renderer/Texture.h"

#include "Povox/Renderer/RenderCommand.h"
#include "Povox/Renderer/Renderer2D.h"

#include "Povox/Utility/Misc.h"

#include <glm/gtc/matrix_transform.hpp>

#define MAX_CUBE_COUNT 20000
#define MAX_VERTEX_COUNT MAX_CUBE_COUNT * 8
#define MAX_INDEX_COUNT MAX_CUBE_COUNT * 6 * 6
#define MAX_TEXTURES 32



namespace Povox {

	struct VoxelRendererData
	{		
		ShaderLibrary ShaderLib;
		Texture2DLibrary Texture2DLib;

		Ref<VertexArray> VoxelVertexArray;

		uint32_t WhiteTexture = 0;
		uint32_t WhiteTextureSlot = 0;

		Vertex* CubeBuffer = nullptr;
		Vertex* CubeBufferPtr = nullptr;

		uint32_t* IndexBuffer = nullptr;
		uint32_t* IndexBufferPtr = nullptr;
		
		uint32_t IndexOffset = 0;
		uint32_t IndexCount = 0;

		std::array<Ref<Texture>, MAX_TEXTURES> TextureSlots;
		uint32_t TextureSlotIndex = 1;	

		VoxelRenderer::Stats Stats;
	};

	static VoxelRendererData* s_VoxelData;

	void VoxelRenderer::Init()
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::Init();
		RenderCommand::SetCulling(true, false);
		Renderer2D::Init();

		s_VoxelData = new VoxelRendererData();
		s_VoxelData->CubeBuffer = new Vertex[MAX_VERTEX_COUNT];
		s_VoxelData->IndexBuffer = new uint32_t[MAX_INDEX_COUNT];

		s_VoxelData->VoxelVertexArray = VertexArray::Create();

		Ref<VertexBuffer> cubeVB = VertexBuffer::CreateBatch(MAX_VERTEX_COUNT * sizeof(Vertex));
		//Set the layout
		cubeVB->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float, "a_TexID" }
		});
		s_VoxelData->VoxelVertexArray->AddVertexBuffer(cubeVB);

		Ref<IndexBuffer> cubeIB = IndexBuffer::CreateBatch(MAX_INDEX_COUNT);
		s_VoxelData->VoxelVertexArray->SetIndexBuffer(cubeIB);

		s_VoxelData->TextureSlots[s_VoxelData->WhiteTextureSlot] = s_VoxelData->Texture2DLib.Load("WhiteBaseTexture", 1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_VoxelData->Texture2DLib.Get("WhiteBaseTexture")->SetData(&whiteTextureData, sizeof(uint32_t));

		for (int i = 1; i < MAX_TEXTURES; i++)
		{
			s_VoxelData->TextureSlots[i] = 0;
		}

		s_VoxelData->ShaderLib.Load("assets/shaders/CubeFlatColored.glsl");		
	}

	void VoxelRenderer::Shutdown()
	{
		PX_PROFILE_FUNCTION();
		

		delete[] s_VoxelData->CubeBuffer;
		delete[] s_VoxelData->IndexBuffer;
		delete s_VoxelData;
	}

	void VoxelRenderer::BeginScene(PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		s_VoxelData->ShaderLib.Get("CubeFlatColored")->Bind();
		s_VoxelData->ShaderLib.Get("CubeFlatColored")->SetMat4("u_ViewProjection", camera.GetViewProjection());
		int samplers[32];
		for (int i = 0; i < MAX_TEXTURES; i++)
		{
			samplers[i] = i;

		}
		s_VoxelData->ShaderLib.Get("CubeFlatColored")->SetIntArray("u_Textures", samplers);
	}

	void VoxelRenderer::EndScene()
	{
		PX_PROFILE_FUNCTION();


	}

	void VoxelRenderer::BeginBatch()
	{
		PX_PROFILE_FUNCTION();


		s_VoxelData->CubeBufferPtr = s_VoxelData->CubeBuffer;
		s_VoxelData->IndexBufferPtr = s_VoxelData->IndexBuffer;
	}

	void VoxelRenderer::EndBatch()
	{
		PX_PROFILE_FUNCTION();


		s_VoxelData->VoxelVertexArray->Bind();
		s_VoxelData->ShaderLib.Get("CubeFlatColored")->Bind();
		s_VoxelData->ShaderLib.Get("CubeFlatColored")->SetMat4("u_Transform", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

		s_VoxelData->VoxelVertexArray->SubmitVertexData(s_VoxelData->CubeBuffer, (uint8_t*)s_VoxelData->CubeBufferPtr - (uint8_t*)s_VoxelData->CubeBuffer);
		s_VoxelData->VoxelVertexArray->SubmitIndices(s_VoxelData->IndexBuffer, (uint8_t*)s_VoxelData->IndexBufferPtr - (uint8_t*)s_VoxelData->IndexBuffer);
	}

	void VoxelRenderer::Flush()
	{
		PX_PROFILE_FUNCTION();


		for (uint32_t i = 0; i < s_VoxelData->TextureSlotIndex; i++)
		{
			s_VoxelData->TextureSlots[i]->Bind(i);
		}

		RenderCommand::DrawIndexed(s_VoxelData->VoxelVertexArray);
		s_VoxelData->Stats.DrawCount++;

		s_VoxelData->IndexOffset = 0;
		s_VoxelData->IndexCount = 0;
		s_VoxelData->TextureSlotIndex = 1;

	}

	//FlatColoredCube
	void VoxelRenderer::DrawCube(glm::vec3 position, float scale, glm::vec4 color)
	{
		if (s_VoxelData->IndexCount >= MAX_INDEX_COUNT)
		{
			EndBatch();
			Flush();
			BeginBatch();
		}

		float textureIndex = 0.0f;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale,  position.y, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale, position.y, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y + scale, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale, position.y + scale, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale, position.y + scale, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y + scale, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		uint32_t offset = s_VoxelData->IndexOffset;

		*s_VoxelData->IndexBufferPtr++ = offset + 0;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 0;

		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 2;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;

		*s_VoxelData->IndexBufferPtr++ = offset + 2;
		*s_VoxelData->IndexBufferPtr++ = offset + 3;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 2;

		*s_VoxelData->IndexBufferPtr++ = offset + 3;
		*s_VoxelData->IndexBufferPtr++ = offset + 0;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 3;

		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;

		*s_VoxelData->IndexBufferPtr++ = offset + 3;
		*s_VoxelData->IndexBufferPtr++ = offset + 2;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 0;
		*s_VoxelData->IndexBufferPtr++ = offset + 3;

		s_VoxelData->IndexOffset += 8;

		s_VoxelData->IndexCount += 36;
		s_VoxelData->Stats.CubeCount++;
		s_VoxelData->Stats.TriangleCount += 12;
	}

	//TexturedCube
	void VoxelRenderer::DrawCube(glm::vec3 position, float scale, const std::string& filepath)
	{
		std::string name = GetNameFromPath(filepath);
		if (s_VoxelData->IndexCount >= MAX_INDEX_COUNT || s_VoxelData->TextureSlotIndex > MAX_TEXTURES - 1)
		{
			EndBatch();
			Flush();
			BeginBatch();
		}

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_VoxelData->TextureSlotIndex; i++)
		{
			if (s_VoxelData->TextureSlots[i]->GetName() == name)
			{
				textureIndex = (float)i;
				break;
			}
		}
		
		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_VoxelData->TextureSlotIndex;
			if (!s_VoxelData->Texture2DLib.Contains(name))
			{
				s_VoxelData->TextureSlots[(int)textureIndex] = s_VoxelData->Texture2DLib.Load(filepath);
			}
			else
			{
				s_VoxelData->TextureSlots[(int)textureIndex] = s_VoxelData->Texture2DLib.Get(name);
			}
			s_VoxelData->TextureSlotIndex++;
		}

		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

			
		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale,  position.y, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale, position.y, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y + scale, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale, position.y + scale, position.z };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x + scale, position.y + scale, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		s_VoxelData->CubeBufferPtr->Position = { position.x, position.y + scale, position.z + scale };
		s_VoxelData->CubeBufferPtr->Color = color;
		s_VoxelData->CubeBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_VoxelData->CubeBufferPtr->TexID = textureIndex;
		s_VoxelData->CubeBufferPtr++;

		uint32_t offset = s_VoxelData->IndexOffset;

		*s_VoxelData->IndexBufferPtr++ = offset + 0;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 0;

		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 2;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;

		*s_VoxelData->IndexBufferPtr++ = offset + 2;
		*s_VoxelData->IndexBufferPtr++ = offset + 3;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 2;

		*s_VoxelData->IndexBufferPtr++ = offset + 3;
		*s_VoxelData->IndexBufferPtr++ = offset + 0;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 3;

		*s_VoxelData->IndexBufferPtr++ = offset + 4;
		*s_VoxelData->IndexBufferPtr++ = offset + 5;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 6;
		*s_VoxelData->IndexBufferPtr++ = offset + 7;
		*s_VoxelData->IndexBufferPtr++ = offset + 4;

		*s_VoxelData->IndexBufferPtr++ = offset + 3;
		*s_VoxelData->IndexBufferPtr++ = offset + 2;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 1;
		*s_VoxelData->IndexBufferPtr++ = offset + 0;
		*s_VoxelData->IndexBufferPtr++ = offset + 3;

		s_VoxelData->IndexOffset += 8;

		s_VoxelData->IndexCount += 36;
		s_VoxelData->Stats.CubeCount++;	
		s_VoxelData->Stats.TriangleCount += 12;
	}

	void VoxelRenderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::SetViewport(0, 0, width, height);
	}

	const VoxelRenderer::Stats& VoxelRenderer::GetStats()
	{
		return s_VoxelData->Stats;
	}

	void VoxelRenderer::ResetStats()
	{
		memset(&s_VoxelData->Stats, 0, sizeof(Stats));
	}
}