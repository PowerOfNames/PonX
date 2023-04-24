#include "pxpch.h"
#include "Povox/Core/UUID.h"

#include <random>

namespace Povox {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;


	UUID::UUID()
		: m_UUID(s_UniformDistribution(s_Engine))
	{
		// TODO: if necessary: m_UUID should not be generated randomly to be 0; -> reserved for unset/invalid ID
	}

	UUID::UUID(uint64_t uuid)
		: m_UUID(uuid)
	{

	}

}
