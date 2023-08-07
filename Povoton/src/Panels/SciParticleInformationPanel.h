#pragma once
#include "Povox/Core/Core.h"
#include "Povox/Core/Log.h"

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
		void DrawParticleNode(SciParticleLayout particle);

	private:
		Povox::Ref<SciParticleSet> m_Context;

		Povox::Ref<SciParticleLayout> m_SelectionContext;
	};


}
