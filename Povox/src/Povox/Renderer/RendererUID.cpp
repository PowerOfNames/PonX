#include "pxpch.h"
#include "Povox/Renderer/RendererUID.h"

#include <random>

namespace Povox {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;


	RendererUID::RendererUID()
		: m_RUID(s_UniformDistribution(s_Engine))
	{
	}

	RendererUID::RendererUID(uint64_t ruid)
		: m_RUID(ruid)
	{
	}

}
