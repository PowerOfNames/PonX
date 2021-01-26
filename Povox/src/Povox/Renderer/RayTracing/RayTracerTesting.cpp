#include "pxpch.h"
#include "Povox/Renderer/RayTracing/RayTracerTesting.h"

#include "Povox/Renderer/RenderCommand.h"
#include "Povox/Renderer/VertexArray.h"

#include "Povox/Renderer/Texture.h"



namespace Povox {

	struct TracerData
	{
		Ref<VertexArray> VertexArray;
		Ref<Shader> RayMarchingShader;

		Ref<Texture> MapData1, MapData2, MapData3;
	};

	static TracerData* s_TracerData;

	void RayTracer::Init()
	{
		PX_PROFILE_FUNCTION();


		RenderCommand::Init();
		s_TracerData = new TracerData();
		s_TracerData->VertexArray = VertexArray::Create();

		float vertices[12] =
		{
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f
		};
		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
		vertexBuffer->SetLayout({
			{ShaderDataType::Float3, "a_Position"}
		});
		s_TracerData->VertexArray->AddVertexBuffer(vertexBuffer);

		uint32_t indices[6] =
		{
			0, 1, 2,
			2, 3, 0
		};

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
		s_TracerData->VertexArray->SetIndexBuffer(indexBuffer);

		s_TracerData->MapData1 = Texture2D::Create("MapData", 8, 64);
		uint32_t mapData1[512];
		for (unsigned int i = 0; i < 512; i++)
		{
			mapData1[i] = 0x00000000;
		}
		mapData1[0] = 0xffffffff;
		mapData1[511] = 0xffffffff;
		s_TracerData->MapData1->SetData(&mapData1, sizeof(uint32_t) * 512);

		s_TracerData->MapData2 = Texture2D::Create("MapData", 1, 2);
		uint32_t mapData2[2];
		for (unsigned int i = 0; i < 2; i++)
		{
			mapData2[i] = 0x22222222;
		}
		mapData2[1] = 0x77007700;
		s_TracerData->MapData2->SetData(&mapData2, sizeof(uint32_t) * 2);


		s_TracerData->MapData3 = Texture2D::Create("MapData", 1, 1);
		uint32_t MapData3[1];
		for (unsigned int i = 0; i < 1; i++)
		{
			MapData3[i] = 0x22222222;
		}
		s_TracerData->MapData3->SetData(&MapData3, sizeof(uint32_t) * 1);

		s_TracerData->RayMarchingShader = Shader::Create("assets/shaders/RayMarchingShader.glsl");
		s_TracerData->RayMarchingShader->Bind();
		//s_TracerData->RayMarchingShader->SetInt("u_MapData", 0);
	}

	void RayTracer::Shutdown()
	{
		delete s_TracerData;
	}

	void RayTracer::BeginScene(PerspectiveCamera& camera, Light& lightsource)
	{
		PX_PROFILE_FUNCTION();

		//s_TracerData->RayMarchingShader->SetMat4("u_ViewProjection", camera.GetViewProjection());
		s_TracerData->RayMarchingShader->SetFloat("u_PointLightIntensity", lightsource.GetIntensity());
		s_TracerData->RayMarchingShader->SetFloat3("u_PointLightPos", lightsource.GetPosition());
		s_TracerData->RayMarchingShader->SetFloat3("u_PointLightColor", lightsource.GetColor());
		s_TracerData->VertexArray->Bind();
	}

	void RayTracer::EndScene()
	{

	}

	void RayTracer::Trace(PerspectiveCameraController& cameraController)
	{
		PX_PROFILE_FUNCTION();

		PerspectiveCamera cam = cameraController.GetCamera();
		s_TracerData->RayMarchingShader->SetFloat3("u_CameraPos", cam.GetPosition());
		s_TracerData->RayMarchingShader->SetFloat3("u_CameraFront", cam.GetFront());
		s_TracerData->RayMarchingShader->SetFloat3("u_CameraUp", cam.GetUp());
		s_TracerData->RayMarchingShader->SetFloat("u_AspectRatio", cam.GetAspectRatio());
		s_TracerData->RayMarchingShader->SetFloat2("u_WindowDims", glm::vec2(cameraController.GetWindowWidth(), cameraController.GetWindowHeight()));
		s_TracerData->RayMarchingShader->SetInt("u_FOV", cameraController.GetFOV());
		s_TracerData->VertexArray->Bind();
		s_TracerData->MapData2->Bind();
		RenderCommand::DrawIndexed(s_TracerData->VertexArray, 6);
	}

}