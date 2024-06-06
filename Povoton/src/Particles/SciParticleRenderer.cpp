#include "pxpch.h"
#include "Povox.h"

#include "SciParticleRenderer.h"

#include <glm/gtc/random.hpp>

namespace Povox {



	SciParticleRenderer::SciParticleRenderer(const SciParticleRendererSpecification& specs)
		: m_Specification(specs)
	{
		//Povox::Renderer::GetShaderLibrary()->Add("DistanceFieldComputeShader", Povox::Shader::Create("assets/shaders/DistanceFieldCompute.glsl"));

		m_ComputeShaderHandle = Povox::Renderer::GetShaderManager()->Load("ComputeTest.glsl");
		m_RayMarchingShaderHandle = Povox::Renderer::GetShaderManager()->Load("RayMarching.glsl");
		m_SilhouetteSimComputeShaderHandle = Povox::Renderer::GetShaderManager()->Load("SilhouetteSimCompute.glsl");
		m_ParticleSilhouetteShaderHandle = Povox::Renderer::GetShaderManager()->Load("ParticleSilhouette.glsl");

		m_DebugShaderHandle = Povox::Renderer::GetShaderManager()->Load("DebugLine.glsl");
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

		m_BillboardCameraData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Mat4, "View" },
			{ Povox::ShaderDataType::Mat4, "InverseView" },
			{ Povox::ShaderDataType::Mat4, "Projection" },
			{ Povox::ShaderDataType::Mat4, "ViewProjection" },
			{ Povox::ShaderDataType::Mat4, "InverseViewProjection" },
			{ Povox::ShaderDataType::Float4, "Forward" },
			{ Povox::ShaderDataType::Float4, "Position" },
			{ Povox::ShaderDataType::Float, "FOV" } }),
			"BillboardCameraUBO"
			);

		m_ActiveCameraData = Povox::CreateRef<Povox::UniformBuffer>(Povox::BufferLayout({
			{ Povox::ShaderDataType::Mat4, "View" },
			{ Povox::ShaderDataType::Mat4, "InverseView" },
			{ Povox::ShaderDataType::Mat4, "Projection" },
			{ Povox::ShaderDataType::Mat4, "ViewProjection" },
			{ Povox::ShaderDataType::Mat4, "InverseViewProjection" },
			{ Povox::ShaderDataType::Float4, "Forward" },
			{ Povox::ShaderDataType::Float4, "Position" },
			{ Povox::ShaderDataType::Float, "FOV" } }),
			"ActiveCameraUBO"
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

		uint32_t maxFrames = Renderer::GetSpecification().MaxFramesInFlight;

		BufferSpecification vertexBufferSpecs{};
		vertexBufferSpecs.Usage = BufferUsage::VERTEX_BUFFER | BufferUsage::STORAGE_BUFFER_DYNAMIC;
		vertexBufferSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		vertexBufferSpecs.ElementCount = m_Specification.MaxParticles;
		vertexBufferSpecs.Size = sizeof(SilhouetteVertex) * m_Specification.MaxParticles * maxFrames;
		vertexBufferSpecs.ElementSize = sizeof(SilhouetteVertex);		
		vertexBufferSpecs.DebugName = "ParticleRenderer Batch Vertexbuffer";
		m_SilhouetteVertexBuffer = Buffer::Create(vertexBufferSpecs);
		m_SilhouetteVertexBufferBase = new SilhouetteVertex[m_Specification.MaxParticles];
		
		m_ParticleSSBOVertex = Povox::CreateRef<Povox::StorageBufferDynamic>(m_Specification.ParticleLayout, m_SilhouetteVertexBuffer, "ParticleSSBOVertex", false);

		m_SilhouetteVertexBufferPtr = m_SilhouetteVertexBufferBase;
		for (uint64_t i = 0; i < m_Specification.MaxParticles; i++)
		{
			m_SilhouetteVertexBufferPtr->PositionRadius = glm::linearRand(glm::vec4(-10.0f, -10.0f, -10.0f, 0.05f), glm::vec4(10.0f, 10.0f, 10.0f, 0.5f));
			m_SilhouetteVertexBufferPtr->Velocity = glm::linearRand(glm::vec4(-1.0, -1.0, -1.0, 0.0), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			m_SilhouetteVertexBufferPtr->Color = glm::linearRand(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f));
			m_SilhouetteVertexBufferPtr++;
		}
		uint64_t dataSize = m_Specification.MaxParticles * sizeof(SilhouetteVertex);
		m_SilhouetteVertexBuffer->SetData(m_SilhouetteVertexBufferBase, dataSize);
		m_SilhouetteVertexBuffer->SetData(m_SilhouetteVertexBufferBase, dataSize, dataSize); // I need to set the second half of the buffer as well to secure the functionality to enable/disable particles

		m_ParticleSSBOVertex->AddDescriptor("ParticleSSBOVertex_0", sizeof(SilhouetteVertex) * m_Specification.MaxParticles, StorageBufferDynamic::FrameBehaviour::FRAME_SWAP_IN_OUT, 0, "ParticleSSBOVertex_1");
		m_ParticleSSBOVertex->AddDescriptor("ParticleSSBOVertex_1", sizeof(SilhouetteVertex) * m_Specification.MaxParticles, StorageBufferDynamic::FrameBehaviour::FRAME_SWAP_IN_OUT, 1, "ParticleSSBOVertex_0");
		// Only needed if the Descriptor is created with an empty buffer and the data is external, i.e. uploaded from disc, generated elsewhere or only available as *void/raw data buffer
		//m_ParticleSSBOVertex->SetDescriptorData("ParticleSSBOVertex_0", m_SilhouetteVertexBuffers[0], sizeof(SilhouetteVertex) * m_Specification.MaxParticles, 0);
		//m_ParticleSSBOVertex->SetDescriptorData("ParticleSSBOVertex_1", nullptr, 0, 0);
		
		BufferSpecification debugVertexSpecs{};
		debugVertexSpecs.Usage = BufferUsage::VERTEX_BUFFER;
		debugVertexSpecs.MemUsage = MemoryUtils::MemoryUsage::GPU_ONLY;
		debugVertexSpecs.ElementCount = 8;
		debugVertexSpecs.Size = sizeof(LineVertex) * 8;
		debugVertexSpecs.ElementSize = sizeof(LineVertex);
		debugVertexSpecs.DebugName = "DebugLinesVertexBuffer";
		m_DebugLinesVertexBuffer = Buffer::Create(debugVertexSpecs);
		m_DebugLinesVertexBufferBase = new LineVertex[8];


		ImageSpecification distanceFieldSpec{};
		distanceFieldSpec.Format = ImageFormat::RED_INTEGER_U32;
		distanceFieldSpec.Width = 2048;
		distanceFieldSpec.Height = 2048;
		distanceFieldSpec.Usages = { ImageUsage::STORAGE, ImageUsage::SAMPLED };
		m_DistanceField = Povox::CreateRef<Povox::StorageImage>(distanceFieldSpec, "DistanceField");

		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)
		glm::mat4 init = glm::mat4(1.0f);
		m_BillboardCameraUniform.View = init;
		m_BillboardCameraUniform.InverseView = init;
		m_BillboardCameraUniform.Projection = init;
		m_BillboardCameraUniform.ViewProjection = init;
		m_BillboardCameraUniform.InverseViewProjection = init;
		m_BillboardCameraUniform.Forward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		m_BillboardCameraUniform.Position = glm::vec4(0.0f);
		m_BillboardCameraUniform.FOV = 90.0f;
		m_BillboardCameraData->SetData((void*)&m_BillboardCameraUniform, sizeof(CameraUniform));

		m_ActiveCameraUniform.View = init;
		m_ActiveCameraUniform.InverseView = init;
		m_ActiveCameraUniform.Projection = init;
		m_ActiveCameraUniform.ViewProjection = init;
		m_ActiveCameraUniform.InverseViewProjection = init;
		m_ActiveCameraUniform.Forward = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		m_ActiveCameraUniform.Position = glm::vec4(0.0f);
		m_ActiveCameraUniform.FOV = 90.0f;
		m_ActiveCameraData->SetData((void*)&m_ActiveCameraUniform, sizeof(CameraUniform));

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
			passSpecs.LocalWorkgroupSize.X = 1;

			m_DistanceFieldComputePass = ComputePass::Create(passSpecs);

			m_DistanceFieldComputePipeline->PrintShaderLayout();

			m_DistanceFieldComputePass->BindInput("ActiveCameraUBO", m_ActiveCameraData);
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
			pipelineSpecs.DebugName = "RaymarchingPipeline";
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

			m_RayMarchingRenderpass->BindInput("ActiveCameraUBO", m_ActiveCameraData);
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

		// Silhouette
		{
			ComputePipelineSpecification pipelineSpecs{};
			pipelineSpecs.DebugName = "SilhouetteSimComputePipeline";
			pipelineSpecs.Shader = Renderer::GetShaderManager()->Get(m_SilhouetteSimComputeShaderHandle);
			m_SilhouetteSimComputePipeline = ComputePipeline::Create(pipelineSpecs);

			ComputePassSpecification passSpecs{};
			passSpecs.DebugName = "SilhouetteSimComputePass";
			passSpecs.DoPerformanceQuery = true;
			passSpecs.Pipeline = m_SilhouetteSimComputePipeline;
			passSpecs.LocalWorkgroupSize.X = 256;
			passSpecs.LocalWorkgroupSize.Y = 1;
			passSpecs.LocalWorkgroupSize.Z = 1;

			m_SilhouetteSimComputePass = ComputePass::Create(passSpecs);

			m_SilhouetteSimComputePass->BindInput("BillboardCameraUBO", m_BillboardCameraData);
			m_SilhouetteSimComputePass->BindInput("RayMarchingUBO", m_RayMarchingData);
			m_SilhouetteSimComputePass->BindInput("ParticleSSBOVertex_0", m_ParticleSSBOVertex);
			m_SilhouetteSimComputePass->BindOutput("ParticleSSBOVertex_1", m_ParticleSSBOVertex);

			m_SilhouetteSimComputePass->Bake();
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
			pipelineSpecs.Primitive = PipelineUtils::PrimitiveTopology::POINT_LIST;
			pipelineSpecs.Shader = Renderer::GetShaderManager()->Get(m_ParticleSilhouetteShaderHandle);
			pipelineSpecs.VertexInputLayout = {
				{ ShaderDataType::Float4, "a_PositionRadius" },
				{ ShaderDataType::Float4, "a_Velocity" },
				{ ShaderDataType::Float4, "a_Color" }
			};
			m_SilhouettePipeline = Pipeline::Create(pipelineSpecs);
			m_SilhouettePipeline->PrintShaderLayout();

			RenderPassSpecification renderpassSpecs{};
			renderpassSpecs.DebugName = "SilhouetteRenderpass";
			renderpassSpecs.DoPerformanceQuery = true;
			renderpassSpecs.TargetFramebuffer = m_SilhouetteFramebuffer;
			renderpassSpecs.Pipeline = m_SilhouettePipeline;

			m_SilhouetteRenderpass = RenderPass::Create(renderpassSpecs);
			m_SilhouetteRenderpass->BindInput("BillboardCameraUBO", m_BillboardCameraData);
			m_SilhouetteRenderpass->BindInput("ActiveCameraUBO", m_ActiveCameraData);
			m_SilhouetteRenderpass->BindInput("ParticleSSBOVertex_1", m_ParticleSSBOVertex);
			m_SilhouetteRenderpass->Bake();

			m_SilhouetteSimComputePass->SetPredecessor(m_SilhouetteRenderpass);
			m_SilhouetteSimComputePass->SetSuccessor(m_SilhouetteRenderpass);
			m_SilhouetteRenderpass->SetPredecessor(m_SilhouetteSimComputePass);
			m_SilhouetteRenderpass->SetSuccessor(m_SilhouetteSimComputePass);
		}

		//Debug
		{
			FramebufferSpecification framebufferSpecs{};
			framebufferSpecs.DebugName = "DebugFramebuffer";
			framebufferSpecs.Attachments = { {ImageFormat::RGBA8}, {ImageFormat::Depth} };
			framebufferSpecs.OriginalImages = { m_SilhouetteFramebuffer->GetColorAttachment(0) };
			framebufferSpecs.Width = m_Specification.ViewportWidth;
			framebufferSpecs.Height = m_Specification.ViewportHeight;
			//m_DebugFramebuffer = Framebuffer::Create(framebufferSpecs);

			PipelineSpecification pipelineSpecs{};
			pipelineSpecs.DebugName = "DebugPipeline";
			pipelineSpecs.TargetFramebuffer = m_SilhouetteFramebuffer;
			pipelineSpecs.DynamicViewAndScissors = true;
			pipelineSpecs.Primitive = PipelineUtils::PrimitiveTopology::LINE_LIST;
			pipelineSpecs.PolygonMode = PipelineUtils::PolygonMode::LINE;
			pipelineSpecs.Shader = Renderer::GetShaderManager()->Get(m_DebugShaderHandle);
			pipelineSpecs.VertexInputLayout = {
				{ ShaderDataType::Float3, "a_Position"},
				{ ShaderDataType::Float4, "a_Color"}
			};
			m_DebugPipeline = Pipeline::Create(pipelineSpecs);
			m_DebugPipeline->PrintShaderLayout();

			RenderPassSpecification renderpassSpecs{};
			renderpassSpecs.DebugName = "DebugLineRenderpass";
			renderpassSpecs.TargetFramebuffer = m_DebugFramebuffer;
			renderpassSpecs.Pipeline = m_DebugPipeline;
			
			//m_DebugRenderpass = RenderPass::Create(renderpassSpecs);
			//m_DebugRenderpass->BindInput("ActiveCameraUBO", m_DebugCameraData);
			//m_DebugRenderpass->Bake();
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
			

		PX_CORE_TRACE("SciRenderer::Init: Completed.");
		return true;
	}
	void SciParticleRenderer::Shutdown()
	{

	}


	void SciParticleRenderer::OnUpdate(float deltaTime, uint64_t totalElements)
	{
		m_ElapsedTime += deltaTime;
		m_DeltaTimne = deltaTime;


		for(auto& [name, set] : m_LoadedParticleSets)
		{
			if (set->GetSpecifications().GPUSimulationActive)
			{
				//Renderer::DispatchCompute(m_DistanceFieldComputePass);
				Renderer::DispatchCompute(m_SilhouetteSimComputePass, totalElements, 16, 16, 4);
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
		//m_DebugRenderpass->Recreate(width, height);
	}


	void SciParticleRenderer::Begin(const EditorCamera& camera)
	{
		PX_PROFILE_FUNCTION();
			

		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)

		m_BillboardCameraUniform.Forward = glm::vec4(camera.GetForwardVector(), 0.0f);
		m_BillboardCameraUniform.Position = glm::vec4(camera.GetPosition(), 0.0f);
		m_BillboardCameraData->SetData((void*)&m_BillboardCameraUniform, sizeof(CameraUniform));
	}


	void SciParticleRenderer::Begin(const PerspectiveCamera& camera)
	{
		PX_PROFILE_FUNCTION();


		//First do the Compute stuff, then wait until compute is finished (barriers connecting ComputePass (THere is no actual computePass, is just to connect resources) and Renderpass)
		m_BillboardCameraUniform.View = camera.GetViewMatrix();
		m_BillboardCameraUniform.InverseView = glm::inverse(camera.GetViewMatrix());
		m_BillboardCameraUniform.Projection = camera.GetProjectionMatrix();
		m_BillboardCameraUniform.ViewProjection = camera.GetViewProjectionMatrix();
		m_BillboardCameraUniform.InverseViewProjection = glm::inverse(camera.GetViewProjectionMatrix());
		m_BillboardCameraUniform.Forward = glm::vec4(camera.GetForward(), 0.0f);
		m_BillboardCameraUniform.Position = glm::vec4(camera.GetPosition(), 0.0f);
		m_BillboardCameraData->SetData((void*)&m_BillboardCameraUniform, sizeof(CameraUniform));

		m_ActiveCameraData->SetData((void*)&m_BillboardCameraUniform, sizeof(CameraUniform));
	}

	void SciParticleRenderer::BeginDebug(const PerspectiveCamera& mainCamera, const PerspectiveCamera& debugCamera)
	{
		m_BillboardCameraUniform.View = mainCamera.GetViewMatrix();
		m_BillboardCameraUniform.InverseView = glm::inverse(mainCamera.GetViewMatrix());
		m_BillboardCameraUniform.Projection = mainCamera.GetProjectionMatrix();
		m_BillboardCameraUniform.ViewProjection = mainCamera.GetViewProjectionMatrix();
		m_BillboardCameraUniform.InverseViewProjection = glm::inverse(mainCamera.GetViewProjectionMatrix());
		m_BillboardCameraUniform.Forward = glm::vec4(mainCamera.GetForward(), 0.0f);
		m_BillboardCameraUniform.Position = glm::vec4(mainCamera.GetPosition(), 0.0f);
		m_BillboardCameraData->SetData((void*)&m_BillboardCameraUniform, sizeof(CameraUniform));

		m_ActiveCameraUniform.View = debugCamera.GetViewMatrix();
		m_ActiveCameraUniform.InverseView = glm::inverse(debugCamera.GetViewMatrix());
		m_ActiveCameraUniform.Projection = debugCamera.GetProjectionMatrix();
		m_ActiveCameraUniform.ViewProjection = debugCamera.GetViewProjectionMatrix(); 
		m_ActiveCameraUniform.InverseViewProjection = glm::inverse(debugCamera.GetViewProjectionMatrix());
		m_ActiveCameraUniform.Forward = glm::vec4(debugCamera.GetForward(), 0.0f);
		m_ActiveCameraUniform.Position = glm::vec4(debugCamera.GetPosition(), 0.0f);
		m_ActiveCameraData->SetData((void*)&m_ActiveCameraUniform, sizeof(CameraUniform));
			

		glm::mat4 invView = glm::inverse(mainCamera.GetViewMatrix());
		m_DebugLinesVertexBufferPtr = m_DebugLinesVertexBufferBase;
		for (const auto& viewCorner : debugCamera.GetFrustumCorners())
		{
			glm::vec4 corner = invView * viewCorner;
			m_DebugLinesVertexBufferPtr->Position = glm::vec3(corner) / corner.w;
			m_DebugLinesVertexBufferPtr->Color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);
			m_DebugLinesVertexBufferPtr++;
		}
		uint64_t dataSize = 8 * sizeof(LineVertex);
		m_DebugLinesVertexBuffer->SetData(m_DebugLinesVertexBufferBase, 8 * sizeof(LineVertex));
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

		Renderer::Draw(m_FullscreenQuadVertexBuffer, 0, m_RayMarchingMaterial, m_FullscreenQuadIndexBuffer, 6, true);

		Renderer::EndRenderPass();
		Renderer::StopTimestampQuery("RaymarchingRenderpass");
		Renderer::EndCommandBuffer();

		m_FinalImage = m_RayMarchingFramebuffer->GetColorAttachment(0);
	}

	void SciParticleRenderer::DrawParticleSilhouette(uint64_t particleCount, bool debug/*= false*/)
	{
		PX_PROFILE_FUNCTION();

		uint32_t currentFrameIndex = Renderer::GetCurrentFrameIndex();
		


		auto cmd = Renderer::GetCommandBuffer(currentFrameIndex);

		Renderer::BeginCommandBuffer(cmd);
		Renderer::StartTimestampQuery("SilhouetteRenderpass");
		Renderer::BeginRenderPass(m_SilhouetteRenderpass);
					
		//uint32_t dataSize = glm::min(particleCount, m_Specification.MaxParticles) * sizeof(SilhouetteVertex);
		//uint32_t dataSize = (uint32_t)((uint8_t*)m_SilhouetteVertexBufferPtr - (uint8_t*)m_SilhouetteVertexBufferBases[currentFrameIndex]);
		//m_SilhouetteVertexBuffers[currentFrameIndex]->SetData(m_SilhouetteVertexBufferBases[0], dataSize);

		Renderer::Draw(m_SilhouetteVertexBuffer, m_Specification.MaxParticles * sizeof(SilhouetteVertex) * currentFrameIndex, nullptr, nullptr, particleCount, true);				

		if (debug)
		{
			//Renderer::BeginRenderPass(m_DebugRenderpass);
			Renderer::BindPipeline(m_DebugPipeline);
			Renderer::Draw(m_DebugLinesVertexBuffer, 0, nullptr, nullptr, 8, true);
			//Renderer::EndRenderPass();
		}
		Renderer::EndRenderPass();
		Renderer::StopTimestampQuery("SilhouetteRenderpass");


		Renderer::EndCommandBuffer();

			m_FinalImage = m_SilhouetteFramebuffer->GetColorAttachment(0);
		//if (debug)
		//	m_FinalImage = m_DebugFramebuffer->GetColorAttachment(0);
		
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
