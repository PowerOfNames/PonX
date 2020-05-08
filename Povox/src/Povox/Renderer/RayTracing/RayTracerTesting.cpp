#include "pxpch.h"
#include "Povox/Renderer/RayTracing/RayTracerTesting.h"

#include "Povox/Renderer/RenderCommand.h"
#include "Povox/Renderer/VertexArray.h"


namespace Povox {
	
	struct TracerData
	{
		Ref<VertexArray> VertexArray;

		Ref<Shader> RayMarchingShader;
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


		s_TracerData->RayMarchingShader = Shader::Create("assets/shaders/RayMarchingShader.glsl");
		s_TracerData->RayMarchingShader->Bind();

	}

	void RayTracer::Shutdown()
	{
		delete s_TracerData;
	}

	void RayTracer::BeginScene(PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();

		//s_TracerData->RayMarchingShader->SetMat4("u_ViewProjection", camera.GetViewProjection());
		//s_TracerData->VertexArray->Bind();
	}

	void RayTracer::EndScene()
	{

	}

	void RayTracer::Trace(PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();

		s_TracerData->RayMarchingShader->SetFloat3("u_CameraPos", camera.GetPosition());
		s_TracerData->RayMarchingShader->SetFloat3("u_CameraFront", camera.GetFront());
		s_TracerData->RayMarchingShader->SetFloat("u_AspectRatio", camera.GetAspectRatio());
		s_TracerData->RayMarchingShader->SetFloat2("u_WindowDims", glm::vec2(camera.GetWidth(), camera.GetHeight()));
		s_TracerData->RayMarchingShader->SetInt("u_FOV", 60);
		s_TracerData->VertexArray->Bind();
		RenderCommand::DrawIndexed(s_TracerData->VertexArray);
	}

}