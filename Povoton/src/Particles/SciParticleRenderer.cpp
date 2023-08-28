#include "pxpch.h"
#include "Povox.h"

#include "SciParticleRenderer.h"

#include "Povox/Utils/ShaderResources.h"



namespace Povox {



	SciParticleRenderer::SciParticleRenderer(const SciParticleRendererSpecification& specs)
		: m_Specification(specs)
	{
		//Povox::Renderer::GetShaderLibrary()->Add("DistanceFieldComputeShader", Povox::Shader::Create("assets/shaders/DistanceFieldCompute.glsl"));
		Povox::Renderer::GetShaderLibrary()->Add("ComputeTestShader", Povox::Shader::Create("assets/shaders/ComputeTest.glsl"));
		Povox::Renderer::GetShaderLibrary()->Add("RayMarchingShader", Povox::Shader::Create("assets/shaders/RayMarching.glsl"));

	}


	bool SciParticleRenderer::Init()
	{
		// Create TimeStamp Query pools here

		PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("SciRenderer::Init: Starting...");

		// General Flow:
		// Create Framebuffers, which store the RenderTraget-Images (Inputs and outputs)
		// Create Pipelines, which define which descriptor sets are used + metadata (winding order, ...)
		// Create Renderpasses, which contain at least one pipeline and wrap a framebuffer, defining barriers between image operations

		// Dependencies: 
		// Renderpass needs Attachment descriptions from the Framebuffer BUT 
		// Framebuffer needs Renderpass handle upon creation -> After the renderpass was created, it triggers the framebuffer creation (vkCreate*() )
		// Renderpass(class) needs set of all needed descriptor sets+bindings (just string identifier) from the shader BUT
		// Pipeline creation triggers extraction of these from the attached shader(s) -> Renderpass triggers vkCreatePipeline call when finished

		// Therefore: 
		// Framebuffer->Create and Pipeline->Create just trigger setup functionality
		// Renderpass->Create + Construct triggers vkCreateRenderpass+Framebuffer+Pipeline in order of demand

		m_CameraData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Mat4, "View" },
			{ Povox::ShaderDataType::Mat4, "Projection" },
			{ Povox::ShaderDataType::Mat4, "ViewProjection" }
			}));

		m_SceneData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Float4 , "FogColor" },
			{ Povox::ShaderDataType::Float4 , "FogDistance" },
			{ Povox::ShaderDataType::Float4 , "AmbientColor" },
			{ Povox::ShaderDataType::Float4 , "SunlightDirection" },
			{ Povox::ShaderDataType::Float4 , "SunlightColor" }})
			);

		m_ParticleData = Povox::CreateRef<Povox::StorageBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Float2, "Position" },
			{ Povox::ShaderDataType::Float2, "Velocity" },
			{ Povox::ShaderDataType::Float3, "Color" },
			{ Povox::ShaderDataType::Long, "ID" }}),
			m_Specification.MaxParticles
			);

		ImageSpecification distanceFieldSpecs{};
		distanceFieldSpecs.Width = 2048;
		distanceFieldSpecs.Height = 2048;
		distanceFieldSpecs.CreateDescriptorOnInit = true;
		distanceFieldSpecs.Format = ImageFormat::RED_INTEGER_U32;
		distanceFieldSpecs.Memory = MemoryUtils::MemoryUsage::GPU_ONLY;
		distanceFieldSpecs.Usages = { ImageUsage::STORAGE };


		uint32_t framesInFlight = Renderer::GetSpecification().MaxFramesInFlight;
		m_ParticleDataSSBOs.resize(framesInFlight);
		m_DistanceFields.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			//m_ParticleDataSSBOs[i] = Buffer::Create(particleBufferSpecs);
			m_DistanceFields[i] = Image2D::Create(distanceFieldSpecs);
		}

		// Compute
// 		{
// 			ComputePipelineSpecification pipelineSpecs{};
// 			pipelineSpecs.DebugName = "DistanceField";
// 			pipelineSpecs.Shader = Renderer::GetShaderLibrary()->Get("ComputeTestShader");
// 			m_DistanceFieldComputePipeline = ComputePipeline::Create(pipelineSpecs);
// 
// 			ComputePassSpecification passSpecs{};
// 			passSpecs.DebugName = "DistanceField";
// 			passSpecs.Pipeline = m_DistanceFieldComputePipeline;
// 			m_DistanceFieldComputePass = ComputePass::Create(passSpecs);
// 		}

		// RayMarch to FullscreenQuad
		{
			FramebufferSpecification framebufferSpecs{};
			framebufferSpecs.DebugName = "RaymarchingFramebuffer";
			framebufferSpecs.Attachments = { {ImageFormat::RGBA8} };
			framebufferSpecs.Width = m_Specification.ViewportWidth;
			framebufferSpecs.Height = m_Specification.ViewportHeight;
			m_RayMarchingFramebuffer = Framebuffer::Create(framebufferSpecs);
			

			PipelineSpecification pipelineSpecs{};
			pipelineSpecs.DebugName = "RayMarchingPipeline";
			pipelineSpecs.Shader = Renderer::GetShaderLibrary()->Get("RayMarchingShader");
			pipelineSpecs.TargetFramebuffer = m_RayMarchingFramebuffer;
			pipelineSpecs.VertexInputLayout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float, "a_TexID" }
			};
			pipelineSpecs.DynamicViewAndScissors = true;
			m_RayMarchingPipeline = Pipeline::Create(pipelineSpecs);


			RenderPassSpecification renderpassSpecs{};
			renderpassSpecs.DebugName = "RaymarchingRenderpass";
			renderpassSpecs.Pipeline = m_RayMarchingPipeline;
			renderpassSpecs.TargetFramebuffer = m_RayMarchingFramebuffer;
			m_RayMarchingRenderpass = RenderPass::Create(renderpassSpecs);
			m_RayMarchingRenderpass->BindResource("CameraData", m_CameraData);
			m_RayMarchingRenderpass->BindResource("SceneData", m_SceneData);
			m_RayMarchingRenderpass->BindResource("ParticlesIn", m_ParticleData);
			m_RayMarchingRenderpass->Bake();

			m_RayMarchingPipeline->PrintShaderLayout();
		}

		glm::vec4 vec = glm::vec4(1.0f);
		m_SceneUniform.AmbientColor = vec;
		m_SceneUniform.FogColor = vec;
		m_SceneUniform.FogDistance = vec;
		m_SceneUniform.SunlightColor = vec;
		m_SceneUniform.SunlightDirection = vec;
		m_SceneData->SetData((void*)&m_SceneUniform, sizeof(SceneUniform));

		m_ParticleUniform.Position = glm::vec2(0.0f);
		m_ParticleUniform.Velocity = glm::vec2(0.0f);
		m_ParticleUniform.Color = glm::vec4(1.0f);
		m_ParticleUniform.ID = 0;
		m_ParticleData->SetData((void*)&m_ParticleUniform, 0, sizeof(ParticleUniform));

		// Fullscreen
		{
			BufferSpecification fullscreenVertexBufferSpecs{};
			fullscreenVertexBufferSpecs.Usage = BufferUsage::VERTEX_BUFFER;
			fullscreenVertexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
			fullscreenVertexBufferSpecs.ElementCount = 4;
			fullscreenVertexBufferSpecs.ElementSize = sizeof(QuadVertex);
			fullscreenVertexBufferSpecs.Size = sizeof(QuadVertex) * 4;
			fullscreenVertexBufferSpecs.DebugName = "Renderer2D FullscreenVertexbuffer";

			m_FullscreenQuadVertexBuffer = Buffer::Create(fullscreenVertexBufferSpecs);

			uint32_t m_FullscreenQuadIndices[6] = { 0, 1, 2, 2, 3, 0 };
			BufferSpecification fullscreenIndexBufferSpecs{};
			fullscreenIndexBufferSpecs.Usage = BufferUsage::INDEX_BUFFER;
			fullscreenIndexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
			fullscreenIndexBufferSpecs.ElementCount = 6;
			fullscreenIndexBufferSpecs.ElementSize = sizeof(uint32_t);
			fullscreenIndexBufferSpecs.Size = sizeof(uint32_t) * 6;
			fullscreenIndexBufferSpecs.DebugName = "Renderer2D FullscreenIndexBuffer";
			m_FullscreenQuadIndexBuffer = Buffer::Create(fullscreenIndexBufferSpecs);
			m_FullscreenQuadIndexBuffer->SetData(m_FullscreenQuadIndices, sizeof(uint32_t) * 6);

			constexpr glm::vec2 textureCoords[4] = { {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f} };
			constexpr glm::vec3 vertexPositions[4] = { { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f } };

			QuadVertex* fullscreenQuadVertices = new QuadVertex[4];
			for (uint32_t i = 0; i < 4; i++)
			{
				fullscreenQuadVertices[i].Position = vertexPositions[i];
				fullscreenQuadVertices[i].Color = glm::vec4(0.0f);
				fullscreenQuadVertices[i].TexCoord = textureCoords[i];
				fullscreenQuadVertices[i].TexID = 0;
			}

			uint32_t dataSize = sizeof(QuadVertex) * 4;
			m_FullscreenQuadVertexBuffer->SetData(fullscreenQuadVertices, dataSize);

			delete[] fullscreenQuadVertices;
		}

		m_RayMarchingMaterial = Material::Create(Renderer::GetShaderLibrary()->Get("RayMarchingShader"), "RayMarching");

		PX_CORE_TRACE("SciRenderer::Init: Completed.");
		return true;
	}
	void SciParticleRenderer::Shutdown()
	{

	}

	void SciParticleRenderer::OnResize(uint32_t width, uint32_t height)
	{
		PX_PROFILE_FUNCTION();

		if ((width <= 0 || height <= 0))
			return;

		if (width == m_Specification.ViewportWidth
			&& height == m_Specification.ViewportHeight)
			return;

		m_Specification.ViewportWidth = width;
		m_Specification.ViewportHeight = height;

		// Compute
		//m_DistanceFieldComputePass->Recreate();
		//m_DistanceFieldComputePipeline->Recreate();

		// RayMarching
		m_RayMarchingRenderpass->Recreate(width, height);
	}


	void SciParticleRenderer::Begin(const EditorCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		

		//First do the Compute stuff, then wait until compute is finished (barries connecting ComputePass (THere is no actual computePass, ist just to connect resources) and Renderpass)

		//m_CameraData.ViewMatrix = camera.GetViewMatrix();
		//m_CameraData.ProjectionMatrix = camera.GetProjectionMatrix();
		//m_CameraData.ViewProjMatrix = camera.GetViewProjectionMatrix();
		//Renderer::UpdateCamera(m_CameraData);

		
		m_CameraUniform.ViewMatrix = camera.GetViewMatrix();
		m_CameraUniform.ProjectionMatrix = camera.GetProjectionMatrix();
		m_CameraUniform.ViewProjMatrix = camera.GetViewProjectionMatrix();

		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));
		//m_CameraData->Set("View", camera.GetViewMatrix());

		glm::vec4 ambientColor = glm::vec4(1.0f);
		m_SceneData->Set((void*)&ambientColor, "AmbientColor", sizeof(float) * 4);

		m_ParticleUniform.Position = glm::vec2(0.0f);
		m_ParticleUniform.Velocity = glm::vec2(0.0f);
		m_ParticleUniform.Color = glm::vec4(0.5f, 0.6f, 0.4f, 1.0f);
		m_ParticleUniform.ID = 0;
		m_ParticleData->SetData((void*)&m_ParticleUniform, 0, sizeof(ParticleUniform));
	}


	void SciParticleRenderer::Flush()
	{
		//Do all the drawings
	}


	void SciParticleRenderer::End()
	{
		PX_PROFILE_FUNCTION();

		Flush();
		FinishRender();				

		m_FinalImage = m_RayMarchingFramebuffer->GetColorAttachment(0);
	}

	/**
	 * This function takes in a particle set and adds its content SSBO to the rendering pipeline. The actual rendering happens during the Flush call.
	 */
	void SciParticleRenderer::DrawParticleSet(Povox::Ref<SciParticleSet> particleSet)
	{
		//	Fullscreen Quad -> rays paint this during ray marching
		PreProcess(particleSet);

		BeginRender();
		Renderer::Draw(m_FullscreenQuadVertexBuffer, m_RayMarchingMaterial, m_FullscreenQuadIndexBuffer, 6);


	}

	void SciParticleRenderer::ResetStatistics()
	{
		m_Statistics.TotalParticles  = 0;
		m_Statistics.RenderedParticles = 0;

		m_Statistics.ParticleSets = 0;
	}

	/**
	 * Do compute shader stuff
	 */
	void SciParticleRenderer::PreProcess(Povox::Ref<SciParticleSet> particleSet)
	{
		PX_PROFILE_FUNCTION();


		//Renderer::BeginComputePass(m_DistanceFieldComputePass);
		//Renderer::DispatchCompute(m_DistanceFieldComputePipeline);
		//Renderer::EndComputePass();
	}

	/**
	 * Start the graphical rendering
	 */
	void SciParticleRenderer::BeginRender()
	{

		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);

		Renderer::BeginCommandBuffer(cmd);
		// Renderer::StartTimestampQuery("RayMarchingRenderpass");
		Renderer::BeginRenderPass(m_RayMarchingRenderpass);

		//Renderer::BindPipeline(m_RayMarchingPipeline);


	}

	/**
	 * Finish the graphical rendering
	 */
	void SciParticleRenderer::FinishRender()
	{
		Renderer::EndRenderPass();
		// Renderer::StopTimestampQuery("RayMarchingRenderpass");
		Renderer::EndCommandBuffer();
	}

}
