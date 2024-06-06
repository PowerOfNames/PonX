#pragma once

#include "SciParticles.h"

#include "Povox.h"

namespace Povox {

	struct SciParticleRendererSpecification
	{
		static const uint64_t MaxParticles = 1024 * 1024;

		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;

		BufferLayout ParticleLayout;
	};

	struct SciParticleRendererStatistics
	{
		uint64_t TotalParticles = 0;
		uint64_t RenderedParticles = 0;

		uint32_t ParticleSets = 0;

		uint64_t ElapsedTimeBetweenFrames = 0;
	};



	class SciParticleRenderer
	{
	public:

		SciParticleRenderer(const SciParticleRendererSpecification& specs);
		~SciParticleRenderer() = default;

		// Core
		bool Init();
		void Shutdown();

		void OnUpdate(float deltaTime, uint64_t totalElements);
		void OnResize(uint32_t width, uint32_t height);

		// Workflow
		void Begin(const EditorCamera& camera);
		void Begin(const PerspectiveCamera& camera);
		void BeginDebug(const PerspectiveCamera& mainCamera, const PerspectiveCamera& debugCamera);
		
		void LoadParticleSet(const std::string& name, Povox::Ref<SciParticleSet> set);
		void DrawParticleSet(Povox::Ref<SciParticleSet> particleSet, uint32_t maxParticleDraws);
		
		void End();

		void DrawParticleSilhouette(uint64_t particleCount, bool debug = false);

		inline const Povox::Ref<Povox::Image2D> GetFinalImage() const { return m_FinalImage; }

		const SciParticleRendererStatistics& GetStatistics() const { return m_Statistics; }
		void ResetStatistics();

	private:


	private:
		SciParticleRendererSpecification m_Specification{};
		float m_ElapsedTime = 0.0f;
		float m_DeltaTimne = 0.0f;

		Povox::Ref<Povox::Image2D> m_FinalImage = nullptr;

		CameraUniform m_BillboardCameraUniform{};
		CameraUniform m_ActiveCameraUniform{};
		RayMarchingUniform m_RayMarchingUniform{};

		Povox::Ref<Povox::UniformBuffer> m_BillboardCameraData = nullptr;
		Povox::Ref<Povox::UniformBuffer> m_ActiveCameraData = nullptr;
		Povox::Ref<Povox::UniformBuffer> m_RayMarchingData = nullptr;
		Povox::Ref<Povox::StorageBufferDynamic> m_ParticleSSBO = nullptr;
		Povox::Ref<Povox::StorageImage> m_DistanceField = nullptr;

		std::unordered_map<std::string, Povox::Ref<SciParticleSet>> m_LoadedParticleSets;

		// Particles
		// Compute
		Povox::Ref<Povox::ComputePass> m_DistanceFieldComputePass = nullptr;
		Povox::Ref<Povox::ComputePipeline> m_DistanceFieldComputePipeline = nullptr;
		 
		Povox::Ref<Povox::ComputePass> m_SilhouetteSimComputePass = nullptr;
		Povox::Ref<Povox::ComputePipeline> m_SilhouetteSimComputePipeline = nullptr;


		// RayMarching
		Povox::Ref<Povox::RenderPass> m_RayMarchingRenderpass = nullptr;
		Povox::Ref<Povox::Framebuffer> m_RayMarchingFramebuffer = nullptr;
		Povox::Ref<Povox::Pipeline> m_RayMarchingPipeline = nullptr;
		Povox::Ref<Povox::Material> m_RayMarchingMaterial = nullptr;

		//Silhouette
		Povox::Ref<Povox::RenderPass> m_SilhouetteRenderpass = nullptr;
		Povox::Ref<Povox::Framebuffer> m_SilhouetteFramebuffer = nullptr;
		Povox::Ref<Povox::Pipeline> m_SilhouettePipeline = nullptr;
		Povox::Ref<Povox::Material> m_SilhouetteMaterial = nullptr;

		Ref<Buffer> m_SilhouetteVertexBuffer = nullptr;
		SilhouetteVertex* m_SilhouetteVertexBufferBase = nullptr;
		SilhouetteVertex* m_SilhouetteVertexBufferPtr = nullptr;

		Povox::Ref<Povox::StorageBufferDynamic> m_ParticleSSBOVertex = nullptr;

		// DebugLines
		Povox::Ref<Povox::RenderPass> m_DebugRenderpass = nullptr;
		Povox::Ref<Povox::Framebuffer> m_DebugFramebuffer = nullptr;
		Povox::Ref<Povox::Pipeline> m_DebugPipeline = nullptr;
		Povox::Ref<Povox::Material> m_DebugMaterial = nullptr;

		Ref<Buffer> m_DebugLinesVertexBuffer = nullptr;
		LineVertex* m_DebugLinesVertexBufferBase = nullptr;
		LineVertex* m_DebugLinesVertexBufferPtr = nullptr;


		Povox::ShaderHandle m_ComputeShaderHandle;
		Povox::ShaderHandle m_SilhouetteSimComputeShaderHandle;
		Povox::ShaderHandle m_DebugShaderHandle;
		Povox::ShaderHandle m_RayMarchingShaderHandle;
		Povox::ShaderHandle m_ParticleSilhouetteShaderHandle;

		// Fullscreen
		Povox::Ref<Povox::Pipeline> m_FullscreenQuadPipeline = nullptr;

		Povox::Ref<Povox::Buffer> m_FullscreenQuadVertexBuffer = nullptr;
		Povox::Ref<Povox::Buffer> m_FullscreenQuadIndexBuffer = nullptr;

		// TODO: here or completely managed by ParticleSet - objects

		// Stats
		SciParticleRendererStatistics m_Statistics{};
	};

}
