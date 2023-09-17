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


		// Renderpass chaining
		// Renderpasses are chained via Predecessor and Successor (Normal and Compute) ( TODO: make them more general with proper abstraction)
		// They can be set via SetPre/Successors() to chain them together or via the specification. The renderer will then take care of the synchronization of attached descriptor resources between
		// the passes.
		// ( The first pass that runs won't have a predecessor, but the last will contain the first as successor )
		//
		// Example:
		//			DistanceField compute pass happens before the RayMarching render

		m_CameraData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Mat4, "View" },
			{ Povox::ShaderDataType::Mat4, "Projection" },
			{ Povox::ShaderDataType::Mat4, "ViewProjection" },
			{ Povox::ShaderDataType::Float4, "Forward" },
			{ Povox::ShaderDataType::Float4, "Position" },
			{ Povox::ShaderDataType::Float, "FOV" } }),
			"CameraDataUBO"
			);

		m_RayMarchingData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{Povox::ShaderDataType::Float4 , "BackgroundColor"},
			{ Povox::ShaderDataType::Float2 , "Resolution" } }),
			"RayMarchingUBO",
			false,
			false
			);

		m_ParticleData = Povox::CreateRef<Povox::StorageBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Float4, "PositionRadius" },
			{ Povox::ShaderDataType::Float4, "Velocity" },
			{ Povox::ShaderDataType::Float4, "Color" },
			{ Povox::ShaderDataType::Long, "ID" }}),
			m_Specification.MaxParticles,
			"ParticleDataSSBO"
			);

		m_DistanceField = Povox::CreateRef<Povox::StorageImage>(ImageFormat::RED_INTEGER_U32, 2048, 2048, ImageUsages({ ImageUsage::STORAGE, ImageUsage::SAMPLED }), "DistanceField");

		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)
		glm::mat4 init = glm::mat4(1.0f);
		m_CameraUniform.View = init;
		m_CameraUniform.Projection = init;
		m_CameraUniform.ViewProjection = init;
		m_CameraUniform.Forward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		m_CameraUniform.Position = glm::vec4(0.0f);
		m_CameraUniform.FOV = 90.0f;
		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));

		m_RayMarchingUniform.BackgroundColor = glm::vec4(0.3f);
		m_RayMarchingUniform.Resolution = glm::vec2((float)m_Specification.ViewportWidth, (float)m_Specification.ViewportHeight);
		m_RayMarchingData->SetData((void*)&m_RayMarchingUniform, sizeof(RayMarchingUniform));

		uint32_t framesInFlight = Renderer::GetSpecification().MaxFramesInFlight;

		// Compute
		{
			ComputePipelineSpecification pipelineSpecs{};
			pipelineSpecs.DebugName = "DistanceField";
			pipelineSpecs.Shader = Renderer::GetShaderLibrary()->Get("ComputeTestShader");
			m_DistanceFieldComputePipeline = ComputePipeline::Create(pipelineSpecs);

			ComputePassSpecification passSpecs{};
			passSpecs.DebugName = "DistanceField";
			passSpecs.Pipeline = m_DistanceFieldComputePipeline;
			m_DistanceFieldComputePass = ComputePass::Create(passSpecs);

			m_DistanceFieldComputePipeline->PrintShaderLayout();

			m_DistanceFieldComputePass->BindResource("CameraData", m_CameraData);
			m_DistanceFieldComputePass->BindResource("RayMarchingData", m_RayMarchingData);
			m_DistanceFieldComputePass->BindResource("ParticlesIn", m_ParticleData);
			//m_DistanceFieldComputePass->Bake();
			//m_DistanceFieldComputePass->BindResource("DistanceField", m_DistanceField);
		}

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
			renderpassSpecs.PredecessorComputePass = m_DistanceFieldComputePass;
			m_RayMarchingRenderpass = RenderPass::Create(renderpassSpecs);

			m_RayMarchingRenderpass->BindResource("CameraData", m_CameraData);
			m_RayMarchingRenderpass->BindResource("RayMarchingData", m_RayMarchingData);
			m_RayMarchingRenderpass->BindResource("ParticlesIn", m_ParticleData);
			m_RayMarchingRenderpass->BindResource("DistanceField", m_DistanceField);
			m_RayMarchingRenderpass->Bake();

			//m_RayMarchingPipeline->PrintShaderLayout();
			m_DistanceFieldComputePass->SetPredecessor(m_DistanceFieldComputePass);
		}

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

			constexpr glm::vec2 textureCoords[4] = { {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f} };
			constexpr glm::vec3 vertexPositions[4] = { { -1.0, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f } };

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


		m_RayMarchingUniform.Resolution = glm::vec2((float)m_Specification.ViewportWidth, (float)m_Specification.ViewportHeight);
		m_RayMarchingData->SetData((void*)&m_RayMarchingUniform, sizeof(RayMarchingUniform));

		// Compute
		//m_DistanceFieldComputePass->Recreate();
		//m_DistanceFieldComputePipeline->Recreate();

		// RayMarching
		m_RayMarchingRenderpass->Recreate(width, height);
	}


	void SciParticleRenderer::Begin(const EditorCamera& camera)
	{
		PX_PROFILE_FUNCTION();
			

		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)

		m_CameraUniform.Forward = glm::vec4(camera.GetForwardVector(), 0.0f);
		m_CameraUniform.Position = glm::vec4(camera.GetPosition(), 0.0f);
		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));
	}


	void SciParticleRenderer::Begin(const PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)
		m_CameraUniform.View = camera.GetViewMatrix();
		m_CameraUniform.Projection = camera.GetProjectionMatrix();
		m_CameraUniform.ViewProjection = camera.GetViewProjectionMatrix();
		m_CameraUniform.Forward = glm::vec4(camera.GetForward(), 0.0f);
		m_CameraUniform.Position = glm::vec4(camera.GetPosition(), 0.0f);
		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));

		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);

		Renderer::BeginCommandBuffer(cmd);
		// Renderer::StartTimestampQuery("RayMarchingRenderpass");
		Renderer::BeginRenderPass(m_RayMarchingRenderpass);
	}


	void SciParticleRenderer::End()
	{
		PX_PROFILE_FUNCTION();

		Renderer::Draw(m_FullscreenQuadVertexBuffer, m_RayMarchingMaterial, m_FullscreenQuadIndexBuffer, 6, true);

		Renderer::EndRenderPass();
		// Renderer::StopTimestampQuery("RayMarchingRenderpass");
		Renderer::EndCommandBuffer();

		m_FinalImage = m_RayMarchingFramebuffer->GetColorAttachment(0);
	}

	/**
	 * This function takes in a particle set and adds its content SSBO to the rendering pipeline. The actual rendering happens during the Flush call.
	 */
	void SciParticleRenderer::DrawParticleSet(Povox::Ref<SciParticleSet> particleSet)
	{
		//	Fullscreen Quad -> rays paint this during ray marching		
		m_ParticleUniform.PositionRadius = glm::vec4(0.0f, 0.0f, -10.0f, 1.0f);
		m_ParticleUniform.Velocity = glm::vec4(0.0f);
		m_ParticleUniform.Color = glm::vec4(0.5f, 0.6f, 0.4f, 1.0f);
		m_ParticleUniform.ID = 0;
		m_ParticleData->SetData((void*)&m_ParticleUniform, 0, sizeof(ParticleUniform));

		//CompteTimestampBegin
		//Renderer::DispatchCompute(m_DistanceFieldComputePass);
		//CompteTimestampEnd
	}

	void SciParticleRenderer::ResetStatistics()
	{
		m_Statistics.TotalParticles  = 0;
		m_Statistics.RenderedParticles = 0;

		m_Statistics.ParticleSets = 0;
	}

}
