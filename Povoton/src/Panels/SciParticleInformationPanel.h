#pragma once
#include "Povox.h"

// TODO: Fix include pathing in project
#include "../Particles/SciParticles.h"


namespace Povox {

	class SciParticleInformationPanel
	{
	public:
		SciParticleInformationPanel() = default;
		SciParticleInformationPanel(const Povox::Ref<SciParticleSet>& context);

		void SetContext(const Povox::Ref<SciParticleSet>& context);

		void OnImGuiRender();

	private:
		void DrawParticleNode(const BufferLayout& particle);

	private:
		Povox::Ref<SciParticleSet> m_Context;

		Povox::Ref<BufferLayout> m_SelectionContext;
	};


}
