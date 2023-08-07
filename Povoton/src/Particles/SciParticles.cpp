#include "SciParticles.h"


namespace Povox {


	/**
	 * Constructor creating an empty ParticleDataSet.
	 */
	SciParticleSet::SciParticleSet(const std::string& debugName /*= "ParticleDataSet_Debug"*/)
		: m_DebugName(debugName)
	{

	}


	SciParticleSet::~SciParticleSet()
	{
		//delete[] m_Data;
	}


	bool SciParticleSet::LoadSet(const std::string& path)
	{
		return false;
	}


	uint64_t SciParticleSet::AddParticle(const SciParticleLayout& particle)
	{
		// TODO: add layout

		m_Properties.ParticleCount++;
		return 0;
	}


	void SciParticleSet::RemoveParticle(long uuid)
	{
		// TODO: find particle in m_Data and remove it

		m_Properties.ParticleCount--;
	}

	Povox::Ref<SciParticleSet> SciParticleSet::CreateCustomSet(uint64_t particleCount, const SciParticleLayout& layout, long rngSeed, const std::string& debugName /*= "CustomParticleSet_Debug"*/)
	{
		Povox::Ref<SciParticleSet> set = Povox::CreateRef<SciParticleSet>(debugName);
		//set->SetLayout(layout);

		// TODO: fill ste here with random particles using the seed -> reproduceability
		for (size_t i = 0; i < particleCount; i++)
		{


			set->AddParticle(layout);
		}


		return set;
	}

}
