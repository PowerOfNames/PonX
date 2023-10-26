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

				
		m_Data = Povox::CreateRef<Povox::StorageBuffer>(specs.ParticleLayout, specs.MaxParticleCount, specs.DebugName, false);

		if (specs.RandomGeneration)
		{
			ParticleUniform* buffer = new ParticleUniform[specs.MaxParticleCount];
			for (uint64_t i = 0; i < specs.MaxParticleCount; i++)
			{
				buffer[i].PositionRadius = glm::linearRand(glm::vec4(-5.0f, -5.0f, -5.0f, 0.1f), glm::vec4(5.0f, 5.0f, 4.0f, 2.0f));
				buffer[i].Color = glm::linearRand(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f));
				buffer[i].Velocity = glm::vec4(1.0f);
				buffer[i].ID = UUID();
				buffer[i].IDPad = 0;
			}
			m_Size = specs.MaxParticleCount * sizeof(ParticleUniform);
			m_ParticleCount = specs.MaxParticleCount;
			m_Data->SetData((void*)buffer, m_Size);

			delete[] buffer;
		}
	}


	SciParticleSet::~SciParticleSet()
	{
		
	}


	void SciParticleSet::OnUpdate(uint32_t deltaTime)
	{
		PX_ASSERT(m_Data, "The particle buffer hasn't been set!");


		/*
		*  Do particle updating here, e.g. generating all particle position deltas, remove or add particles according to life cycle (not implemented...)...
		*/
	}

	/*bool SciParticleSet::LoadSet(const std::string& path)
	{
		return false;
	}*/

}
