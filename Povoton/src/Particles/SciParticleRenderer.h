#pragma once

#include "SciParticles.h"

#include "Povox.h"

namespace Povox {

	struct SciParticleRendererSpecification
	{
		static const uint64_t MaxParticles = 100000;

		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
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

		void OnResize(uint32_t width, uint32_t height);

		// Workflow
		void Begin(const EditorCamera& camera);
		
		void DrawParticleSet(Povox::Ref<SciParticleSet> particleSet);
		
		void Flush();
		void End();

		inline const Povox::Ref<Povox::Image2D> GetFinalImage() const { return m_FinalImage; }

		const SciParticleRendererStatistics& GetStatistics() const { return m_Statistics; }
		void ResetStatistics();

	private:
		void PreProcess(Povox::Ref<SciParticleSet> particleSet);
		void BeginRender();
		void FinishRender();


	private:
		SciParticleRendererSpecification m_Specification{};	

		Povox::Ref<Povox::Image2D> m_FinalImage = nullptr;

		CameraUniform m_CameraUniform{};
		SceneUniform m_SceneUniform{};
		ParticleUniform m_ParticleUniform{};

		Povox::Ref<UniformBuffer> m_CameraData = nullptr;
		Povox::Ref<UniformBuffer> m_SceneData = nullptr;
		Povox::Ref<StorageBuffer> m_ParticleData = nullptr;
		std::vector<Povox::Ref<Povox::Buffer>> m_ParticleDataSSBOs;


		// Particles
		// Compute
		Povox::Ref<Povox::ComputePass> m_DistanceFieldComputePass = nullptr;
		Povox::Ref<Povox::ComputePipeline> m_DistanceFieldComputePipeline = nullptr;
		 
		// RayMarching
		Povox::Ref<Povox::RenderPass> m_RayMarchingRenderpass = nullptr;
		Povox::Ref<Povox::Framebuffer> m_RayMarchingFramebuffer = nullptr;
		Povox::Ref<Povox::Pipeline> m_RayMarchingPipeline = nullptr;
		Povox::Ref<Povox::Material> m_RayMarchingMaterial = nullptr;

		// Fullscreen
		Povox::Ref<Povox::Pipeline> m_FullscreenQuadPipeline = nullptr;

		Povox::Ref<Povox::Buffer> m_FullscreenQuadVertexBuffer = nullptr;
		Povox::Ref<Povox::Buffer> m_FullscreenQuadIndexBuffer = nullptr;

		// TODO: here or completely managed by ParticleSet - objects
		std::vector<Povox::Ref<Povox::Image2D>> m_DistanceFields;

		// Stats
		SciParticleRendererStatistics m_Statistics{};
	};

}
