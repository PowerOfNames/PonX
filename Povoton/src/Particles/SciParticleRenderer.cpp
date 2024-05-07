#include "pxpch.h"
#include "Povox.h"

#include "SciParticleRenderer.h"

#include <glm/gtc/random.hpp>

namespace Povox {



	SciParticleRenderer::SciParticleRenderer(const SciParticleRendererSpecification& specs)
		: m_Specification(specs)
	{
		//Povox::Renderer::GetShaderLibrary()->Add("DistanceFieldComputeShader", Povox::Shader::Create("assets/shaders/DistanceFieldCompute.glsl"));
		//Povox::Renderer::GetShaderManager()->Add("ComputeTest", Povox::Shader::Create(std::filesystem::path("assets/shaders/ComputeTest.glsl")));
		//Povox::Renderer::GetShaderManager()->Add("RayMarching", Povox::Shader::Create(std::filesystem::path("assets/shaders/RayMarching.glsl")));

		m_ComputeShaderHandle = Povox::Renderer::GetShaderManager()->Load("ComputeTest.glsl");
		m_RayMarchingShaderHandle = Povox::Renderer::GetShaderManager()->Load("RayMarching.glsl");
		m_ParticleSilhouetteShaderHandle = Povox::Renderer::GetShaderManager()->Load("ParticleSilhouette.glsl");
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
			{ Povox::ShaderDataType::Mat4, "InverseView" },
			{ Povox::ShaderDataType::Mat4, "Projection" },
			{ Povox::ShaderDataType::Mat4, "ViewProjection" },
			{ Povox::ShaderDataType::Float4, "Forward" },
			{ Povox::ShaderDataType::Float4, "Position" },
			{ Povox::ShaderDataType::Float, "FOV" } }),
			"CameraUBO"
			);

		m_RayMarchingData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Float4 , "BackgroundColor"},
			{ Povox::ShaderDataType::Float4 , "ResolutionTime"},
			{ Povox::ShaderDataType::ULong , "ParticleCount"} }),
			"RayMarchingUBO"			
			);

		m_ParticleSSBO = Povox::CreateRef<Povox::StorageBufferDynamic>(m_Specification.ParticleLayout, 
			1024*1024,
			"ParticleDataSSBO",
			false);
				
		ImageSpecification distanceFieldSpec{};
		distanceFieldSpec.Format = ImageFormat::RED_INTEGER_U32;
		distanceFieldSpec.Width = 2048;
		distanceFieldSpec.Height = 2048;
		distanceFieldSpec.Usages = { ImageUsage::STORAGE, ImageUsage::SAMPLED };
		m_DistanceField = Povox::CreateRef<Povox::StorageImage>(distanceFieldSpec, "DistanceField");

		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)
		glm::mat4 init = glm::mat4(1.0f);
		m_CameraUniform.View = init;
		m_CameraUniform.InverseView = init;
		m_CameraUniform.Projection = init;
		m_CameraUniform.ViewProjection = init;
		m_CameraUniform.Forward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		m_CameraUniform.Position = glm::vec4(0.0f);
		m_CameraUniform.FOV = 90.0f;
		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));

		m_RayMarchingUniform.BackgroundColor = glm::vec4(0.3f);
		m_RayMarchingUniform.ResolutionTime = glm::vec4((float)m_Specification.ViewportWidth, (float)m_Specification.ViewportHeight, 0.0f, 0.0f);
		m_RayMarchingUniform.ParticleCount = 1;
		m_RayMarchingData->SetData((void*)&m_RayMarchingUniform, sizeof(RayMarchingUniform));

		uint32_t framesInFlight = Renderer::GetSpecification().MaxFramesInFlight;

		// Compute
		{
			ComputePipelineSpecification pipelineSpecs{};
			pipelineSpecs.DebugName = "ParticleMovementComputePipeline";
			pipelineSpecs.Shader = Renderer::GetShaderManager()->Get(m_ComputeShaderHandle);
			m_DistanceFieldComputePipeline = ComputePipeline::Create(pipelineSpecs);

			ComputePassSpecification passSpecs{};
			passSpecs.DebugName = "ParticleMovementComputePass";
			passSpecs.DoPerformanceQuery = true;
			passSpecs.Pipeline = m_DistanceFieldComputePipeline;
			passSpecs.WorkgroupSize.X = 1;

			m_DistanceFieldComputePass = ComputePass::Create(passSpecs);

			m_DistanceFieldComputePipeline->PrintShaderLayout();

			m_DistanceFieldComputePass->BindInput("CameraUBO", m_CameraData);
			m_DistanceFieldComputePass->BindInput("RayMarchingUBO", m_RayMarchingData);
			m_DistanceFieldComputePass->BindInput("ParticleSSBOIn", m_ParticleSSBO);
			m_DistanceFieldComputePass->BindOutput("ParticleSSBOOut", m_ParticleSSBO);
			
			//m_DistanceFieldComputePass->BindOutput("DistanceField", m_DistanceField);
			m_DistanceFieldComputePass->Bake();
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
			pipelineSpecs.Shader = Renderer::GetShaderManager()->Get(m_RayMarchingShaderHandle);
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
			renderpassSpecs.DoPerformanceQuery = true;
			renderpassSpecs.Pipeline = m_RayMarchingPipeline;
			renderpassSpecs.TargetFramebuffer = m_RayMarchingFramebuffer;
			m_RayMarchingRenderpass = RenderPass::Create(renderpassSpecs);

			m_RayMarchingRenderpass->BindInput("CameraUBO", m_CameraData);
			m_RayMarchingRenderpass->BindInput("RayMarchingUBO", m_RayMarchingData);
			m_RayMarchingRenderpass->BindInput("ParticleSSBOIn", m_ParticleSSBO);
			m_RayMarchingRenderpass->BindInput("DistanceField", m_DistanceField);
			m_RayMarchingRenderpass->Bake();

			//m_RayMarchingPipeline->PrintShaderLayout();

			//TODO: this should be done in a RenderGraph(manager)
			m_RayMarchingRenderpass->SetPredecessor(m_DistanceFieldComputePass);
			m_RayMarchingRenderpass->SetSuccessor(m_DistanceFieldComputePass);
			m_DistanceFieldComputePass->SetPredecessor(m_RayMarchingRenderpass);
			m_DistanceFieldComputePass->SetSuccessor(m_RayMarchingRenderpass);
		}

		{
			FramebufferSpecification framebufferSpecs{};
			framebufferSpecs.DebugName = "SilhouetteFramebuffer";
			framebufferSpecs.Attachments = { {ImageFormat::RGBA8}, {ImageFormat::Depth} };
			framebufferSpecs.Width = m_Specification.ViewportWidth;
			framebufferSpecs.Height = m_Specification.ViewportHeight;
			m_SilhouetteFramebuffer= Framebuffer::Create(framebufferSpecs);

			PipelineSpecification pipelineSpecs{};
			pipelineSpecs.DebugName = "SilhouettePipeline";
			pipelineSpecs.TargetFramebuffer = m_SilhouetteFramebuffer;
			pipelineSpecs.DynamicViewAndScissors = true;
			pipelineSpecs.Culling = PipelineUtils::CullMode::BACK;
			pipelineSpecs.Primitive = PipelineUtils::PrimitiveTopology::POINT_LIST;
			pipelineSpecs.Shader = Renderer::GetShaderManager()->Get(m_ParticleSilhouetteShaderHandle);
			pipelineSpecs.VertexInputLayout = {
				{ ShaderDataType::Float3, "a_PositionRadius" },
				{ ShaderDataType::Float4, "a_Color" }
			};
			m_SilhouettePipeline = Pipeline::Create(pipelineSpecs);

			RenderPassSpecification renderpassSpecs{};
			renderpassSpecs.DebugName = "SilhouetteRenderpass";
			renderpassSpecs.DoPerformanceQuery = true;
			renderpassSpecs.TargetFramebuffer = m_SilhouetteFramebuffer;
			renderpassSpecs.Pipeline = m_SilhouettePipeline;

			m_SilhouetteRenderpass = RenderPass::Create(renderpassSpecs);
			m_SilhouetteRenderpass->BindInput("CameraUBO", m_CameraData);
			m_SilhouetteRenderpass->Bake();

			m_SilhouettePipeline->PrintShaderLayout();
		}

		// Fullscreen
		{
			BufferSpecification fullscreenVertexBufferSpecs{};
			fullscreenVertexBufferSpecs.Usage = BufferUsage::VERTEX_BUFFER;
			fullscreenVertexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
			fullscreenVertexBufferSpecs.ElementCount = 4;
			fullscreenVertexBufferSpecs.Size = sizeof(QuadVertex) * 4;
			fullscreenVertexBufferSpecs.DebugName = "Renderer2D FullscreenVertexbuffer";

			m_FullscreenQuadVertexBuffer = Buffer::Create(fullscreenVertexBufferSpecs);

			uint32_t m_FullscreenQuadIndices[6] = { 0, 1, 2, 2, 3, 0 };
			BufferSpecification fullscreenIndexBufferSpecs{};
			fullscreenIndexBufferSpecs.Usage = BufferUsage::INDEX_BUFFER_32;
			fullscreenIndexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
			fullscreenIndexBufferSpecs.ElementCount = 6;
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

		m_RayMarchingMaterial = Material::Create(Renderer::GetShaderManager()->Get(m_RayMarchingShaderHandle), "RayMarching");

		BufferSpecification vertexBufferSpecs{};
		vertexBufferSpecs.Usage = BufferUsage::VERTEX_BUFFER;
		vertexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		vertexBufferSpecs.ElementCount = m_Specification.MaxParticles;
		vertexBufferSpecs.Size = sizeof(SilhouetteVertex) * m_Specification.MaxParticles;

		uint32_t maxFrames = Renderer::GetSpecification().MaxFramesInFlight;
		m_SilhouetteVertexBuffers.resize(maxFrames);
		m_SilhouetteVertexBufferBases.resize(maxFrames);
		for (uint32_t i = 0; i < maxFrames; i++)
		{
			vertexBufferSpecs.DebugName = "ParticleRenderer Batch Vertexbuffer Frame: " + std::to_string(i);
			m_SilhouetteVertexBuffers[i] = Buffer::Create(vertexBufferSpecs);
			m_SilhouetteVertexBufferBases[i] = new SilhouetteVertex[m_Specification.MaxParticles];			
		}

		m_SilhouetteVertexBufferPtr = m_SilhouetteVertexBufferBases[0];
		for (uint64_t i = 0; i < m_Specification.MaxParticles; i++)
		{
			m_SilhouetteVertexBufferPtr->PositionRadius = glm::linearRand(glm::vec4(-10.0f, -10.0f, -10.0f, 0.05f), glm::vec4(10.0f, 10.0f, 10.0f, 0.5f));
			m_SilhouetteVertexBufferPtr->Color = glm::linearRand(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f));
			m_SilhouetteVertexBufferPtr++;
		}

		PX_CORE_TRACE("SciRenderer::Init: Completed.");
		return true;
	}
	void SciParticleRenderer::Shutdown()
	{

	}


	void SciParticleRenderer::OnUpdate(float deltaTime)
	{
		m_ElapsedTime += deltaTime;
		m_DeltaTimne = deltaTime;


		for(auto& [name, set] : m_LoadedParticleSets)
		{
			if (set->GetSpecifications().GPUSimulationActive)
			{
				//Renderer::StartTimestampQuery("ParticleMovementComputePass");
				Renderer::DispatchCompute(m_DistanceFieldComputePass);
				//Renderer::StopTimestampQuery("ParticleMovementComputePass");
			}
		}
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


		m_RayMarchingUniform.ResolutionTime = glm::vec4((float)m_Specification.ViewportWidth, (float)m_Specification.ViewportHeight, m_ElapsedTime, 0.0f);
		m_RayMarchingData->SetData((void*)&m_RayMarchingUniform, sizeof(RayMarchingUniform));

		// Compute
		//m_DistanceFieldComputePass->Recreate();
		//m_DistanceFieldComputePipeline->Recreate();

		// RayMarching
		m_RayMarchingRenderpass->Recreate(width, height);
		m_SilhouetteRenderpass->Recreate(width, height);
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
		m_CameraUniform.InverseView = glm::inverse(camera.GetViewMatrix());
		m_CameraUniform.Projection = camera.GetProjectionMatrix();
		m_CameraUniform.ViewProjection = camera.GetViewProjectionMatrix();
		m_CameraUniform.Forward = glm::vec4(camera.GetForward(), 0.0f);
		m_CameraUniform.Position = glm::vec4(camera.GetPosition(), 0.0f);
		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));
	}


	void SciParticleRenderer::LoadParticleSet(const std::string& name, Povox::Ref<SciParticleSet> set)
	{
		m_LoadedParticleSets[name] = set;


		m_ParticleSSBO->AddDescriptor("ParticleSSBOIn", set->GetSize(), StorageBufferDynamic::FrameBehaviour::FRAME_SWAP_IN_OUT, 0, "ParticleSSBOOut");
		m_ParticleSSBO->SetDescriptorData("ParticleSSBOIn", set->GetDataBuffer(), set->GetSize(), 0);
		m_ParticleSSBO->AddDescriptor("ParticleSSBOOut", set->GetSize(), StorageBufferDynamic::FrameBehaviour::FRAME_SWAP_IN_OUT, 1, "ParticleSSBOIn");
		m_ParticleSSBO->SetDescriptorData("ParticleSSBOOut", nullptr, 0, 0);

		m_DistanceFieldComputePass->UpdateDescriptor("ParticleSSBOIn");
		m_DistanceFieldComputePass->UpdateDescriptor("ParticleSSBOOut");
		m_RayMarchingRenderpass->UpdateDescriptor("ParticleSSBOIn");
	}

	void SciParticleRenderer::End()
	{
		PX_PROFILE_FUNCTION();



		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);
		Renderer::BeginCommandBuffer(cmd);
		Renderer::StartTimestampQuery("RaymarchingRenderpass");
		Renderer::BeginRenderPass(m_RayMarchingRenderpass);

		Renderer::Draw(m_FullscreenQuadVertexBuffer, m_RayMarchingMaterial, m_FullscreenQuadIndexBuffer, 6, true);

		Renderer::EndRenderPass();
		Renderer::StopTimestampQuery("RaymarchingRenderpass");
		Renderer::EndCommandBuffer();

		m_FinalImage = m_RayMarchingFramebuffer->GetColorAttachment(0);
	}

	void SciParticleRenderer::DrawParticleSilhouette(const PerspectiveCamera& camera, uint64_t particleCount)
	{
		PX_PROFILE_FUNCTION();


		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)
		m_CameraUniform.View = camera.GetViewMatrix();
		m_CameraUniform.InverseView = glm::inverse(camera.GetViewMatrix());
		m_CameraUniform.Projection = camera.GetProjectionMatrix();
		m_CameraUniform.ViewProjection = camera.GetViewProjectionMatrix();
		m_CameraUniform.Forward = glm::vec4(camera.GetForward(), 0.0f);
		m_CameraUniform.Position = glm::vec4(camera.GetPosition(), 0.0f);
		m_CameraData->SetData((void*)&m_CameraUniform, sizeof(CameraUniform));


		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		


		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);

		Renderer::BeginCommandBuffer(cmd);
		Renderer::StartTimestampQuery("SilhouetteRenderpass");
		Renderer::BeginRenderPass(m_SilhouetteRenderpass);

		if (particleCount > 0)
		{			
			uint32_t dataSize = glm::min(particleCount, m_Specification.MaxParticles) * sizeof(SilhouetteVertex);
			//uint32_t dataSize = (uint32_t)((uint8_t*)m_SilhouetteVertexBufferPtr - (uint8_t*)m_SilhouetteVertexBufferBases[currentFrameIndex]);
			m_SilhouetteVertexBuffers[currentFrameIndex]->SetData(m_SilhouetteVertexBufferBases[0], dataSize);

			Renderer::Draw(m_SilhouetteVertexBuffers[currentFrameIndex], nullptr, nullptr, particleCount, true);
		}		

		Renderer::EndRenderPass();
		Renderer::StopTimestampQuery("SilhouetteRenderpass");
		Renderer::EndCommandBuffer();

		m_FinalImage = m_SilhouetteFramebuffer->GetColorAttachment(0);
	}

	/**
	 * This function takes in a particle set and adds its content SSBO to the rendering pipeline. The actual rendering happens during the Flush call.
	 */
	void SciParticleRenderer::DrawParticleSet(Povox::Ref<SciParticleSet> particleSet, uint32_t maxParticleDraws)
	{	
		m_RayMarchingUniform.ResolutionTime.z = m_DeltaTimne;
		m_RayMarchingUniform.ParticleCount = maxParticleDraws;
		//m_RayMarchingUniform.ParticleCount = particleSet->GetParticleCount();

		m_RayMarchingData->SetData((void*)&m_RayMarchingUniform, sizeof(RayMarchingUniform));
			
	}

	void SciParticleRenderer::ResetStatistics()
	{
		m_Statistics.TotalParticles  = 0;
		m_Statistics.RenderedParticles = 0;

		m_Statistics.ParticleSets = 0;
	}

}
