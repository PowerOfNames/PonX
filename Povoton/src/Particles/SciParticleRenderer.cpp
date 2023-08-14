#include "pxpch.h"
#include "SciParticleRenderer.h"

#include "Povox.h"

namespace Povox {



	SciParticleRenderer::SciParticleRenderer(const SciParticleRendererSpecification& specs)
		: m_Specification(specs)
	{
		//Povox::Renderer::GetShaderLibrary()->Add("DistanceFieldComputeShader", Povox::Shader::Create("assets/shaders/DistanceFieldCompute.glsl"));
		Povox::Renderer::GetShaderLibrary()->Add("ComputeTestShader", Povox::Shader::Create("assets/shaders/ComputeTest.glsl"));
		Povox::Renderer::GetShaderLibrary()->Add("RayMarchingShader", Povox::Shader::Create("assets/shaders/RayMarching.glsl"));


		// Compute
		{

		}

		// RayMarch to FullscreenQuad
		{
			FramebufferSpecification rayMarchingFBSpecs{};
			rayMarchingFBSpecs.DebugName = "RaymarchingFramebuffer";
			rayMarchingFBSpecs.Attachments = { {ImageFormat::RGBA8} };
			rayMarchingFBSpecs.Width = m_Specification.ViewportWidth;
			rayMarchingFBSpecs.Height = m_Specification.ViewportHeight;
			rayMarchingFBSpecs.SwapChainTarget = false;
			m_RayMarchingFramebuffer = Framebuffer::Create(rayMarchingFBSpecs);


			RenderPassSpecification rayMarchingRPSpecs{};
			rayMarchingRPSpecs.DebugName = "RaymarchingRenderpass";
			rayMarchingRPSpecs.TargetFramebuffer = m_RayMarchingFramebuffer;
			rayMarchingRPSpecs.ColorAttachmentCount = 1;
			rayMarchingRPSpecs.HasDepthAttachment = false;
			m_RayMarchingRenderpass = RenderPass::Create(rayMarchingRPSpecs);


			PipelineSpecification rayMarchingPLSpecs{};
			rayMarchingPLSpecs.DebugName = "RayMarchingPipeline";
			rayMarchingPLSpecs.DynamicViewAndScissors = true;
			rayMarchingPLSpecs.Culling = PipelineUtils::CullMode::BACK;
			rayMarchingPLSpecs.TargetRenderPass = m_RayMarchingRenderpass;
			rayMarchingPLSpecs.Shader = Renderer::GetShaderLibrary()->Get("RayMarchingShader");
			m_RayMarchingPipeline = Pipeline::Create(rayMarchingPLSpecs);
		}
	}


	bool SciParticleRenderer::Init()
	{
		// Create TimeStamp Query pools here

		PX_PROFILE_FUNCTION();


		PX_CORE_TRACE("SciRenderer::Init: Starting...");

		BufferSpecification particleBufferSpecs{};
		particleBufferSpecs.Usage = BufferUsage::STORAGE_BUFFER;
		particleBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		particleBufferSpecs.ElementCount = m_Specification.MaxParticles;
		particleBufferSpecs.ElementSize = sizeof(SciParticle);
		particleBufferSpecs.Size = sizeof(SciParticle) * m_Specification.MaxParticles;
		particleBufferSpecs.SetLayout({
				{ ShaderDataType::Float2, "Position" },
				{ ShaderDataType::Float2, "Velocity" },
				{ ShaderDataType::Float3, "Color" },
				{ ShaderDataType::Long, "ID" }
			});

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
			particleBufferSpecs.DebugName = "SciParticleRendererBuffer Frame: " + std::to_string(i);

			m_ParticleDataSSBOs[i] = Buffer::Create(particleBufferSpecs);
			m_DistanceFields[i] = Image2D::Create(distanceFieldSpecs);
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
			fullscreenIndexBufferSpecs.Data = m_FullscreenQuadIndices;
			fullscreenIndexBufferSpecs.DebugName = "Renderer2D FullscreenIndexBuffer";

			m_FullscreenQuadIndexBuffer = Buffer::Create(fullscreenIndexBufferSpecs);

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
		m_RayMarchingFramebuffer->Recreate(width, height);
		m_RayMarchingRenderpass->Recreate();
		m_RayMarchingPipeline->Recreate();
	}


	void SciParticleRenderer::Begin(const EditorCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		

		//First do the Compute stuff, then wait until compute is finished (barries connecting ComputePass (THere is no actual computePass, ist just to connect resources) and Renderpass)

		m_CameraData.ViewMatrix = camera.GetViewMatrix();
		m_CameraData.ProjectionMatrix = camera.GetProjectionMatrix();
		m_CameraData.ViewProjMatrix = camera.GetViewProjectionMatrix();
		Renderer::UpdateCamera(m_CameraData);

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

		Renderer::BindPipeline(m_RayMarchingPipeline);


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
