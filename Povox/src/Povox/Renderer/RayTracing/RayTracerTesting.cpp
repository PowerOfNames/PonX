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

		Ref<Texture> MapData;
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

		s_TracerData->MapData = Texture2D::Create("MapData", 8, 64);
		uint32_t mapData[512];
		for (unsigned int i = 0; i < 512; i++)
		{
			mapData[i] = 0xffffffff;
		}
		mapData[0] = 0xffffffff;
		mapData[511] = 0xffffffff;
		s_TracerData->MapData->SetData(&mapData, sizeof(uint32_t) * 512);

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
		s_TracerData->RayMarchingShader->SetFloat3("u_PointLightPos", lightsource.GetPosition());
		s_TracerData->RayMarchingShader->SetFloat3("u_PointLightColor", lightsource.GetColor());
		s_TracerData->RayMarchingShader->SetFloat("u_PointLightIntensity", lightsource.GetIntensity());
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
		RenderCommand::DrawIndexed(s_TracerData->VertexArray);
	}

}