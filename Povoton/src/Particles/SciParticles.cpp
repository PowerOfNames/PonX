#include "SciParticles.h"
#include "Povox.h"

#include <glm/gtc/random.hpp>

namespace Povox {

	/**
	 * Constructor creating an empty ParticleDataSet.
	 */
	SciParticleSet::SciParticleSet(const SciParticleSetSpecification& specs)
		: m_Specification(specs)
	{
		PX_PROFILE_FUNCTION();

		uint32_t frames = Application::Get()->GetSpecification().MaxFramesInFlight;

		if (specs.RandomGeneration)
		{
			ParticleUniform* buffer = new ParticleUniform[specs.MaxParticleCount];
			m_Size = specs.MaxParticleCount * sizeof(ParticleUniform);
			m_Data = new uint8_t[m_Size];

			for (uint64_t i = 0; i < specs.MaxParticleCount; i++)
			{
				buffer[i].PositionRadius = glm::linearRand(glm::vec4(-5.0f, -5.0f, -5.0f, 0.1f), glm::vec4(5.0f, 5.0f, 4.0f, 2.0f));
				buffer[i].Color = glm::linearRand(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f));
				buffer[i].Velocity = glm::linearRand(-glm::vec4(5.0f), glm::vec4(5.0f));
				buffer[i].ID = UUID();
				buffer[i].IDPad = 0;
			}
			m_ParticleCount = specs.MaxParticleCount;


			memcpy(m_Data, (uint8_t*)buffer, m_Size);
			//m_Data->SetData((void*)buffer, 0, m_Size);

			delete[] buffer;
		}
	}


	SciParticleSet::~SciParticleSet()
	{
		
	}


	void SciParticleSet::OnUpdate(uint32_t deltaTime)
	{
		//PX_ASSERT(m_Data, "The particle buffer hasn't been set!");

		uint32_t current = Renderer::GetCurrentFrameIndex();
		uint32_t last = Renderer::GetLastFrameIndex();


		//m_Buffer->SetData();
	}

// 	/*Povox::Ref<Povox::StorageBufferDynamic> SciParticleSet::GetDataBuffer(uint32_t frame)
// 	{
// 		PX_ASSERT(frame < m_Data.size(), "Frame index out of scope!");
// 
// 		return m_Data[frame];
// 	}
// 	const Povox::Ref<Povox::StorageBufferDynamic>& SciParticleSet::GetDataBuffer(uint32_t frame) const
// 	{
// 		return SciParticleSet::GetDataBuffer(frame);
// 	}*/

	/*bool SciParticleSet::LoadSet(const std::string& path)
	{
		return false;
	}*/

}
