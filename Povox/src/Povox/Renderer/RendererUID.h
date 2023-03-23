#pragma once

#include <xhash>

namespace Povox {

	/**
	 * Used for Entities in the Entity Component System. Use [RendererUID]
	 */
	class RendererUID
	{
	public:
		RendererUID();
		RendererUID(uint64_t ruid);
		RendererUID(const RendererUID& ruid) = default;

		operator uint64_t() const { return m_RUID; }
	private:
		uint64_t m_RUID;
	};
}


namespace std {

	template<>
	struct hash<Povox::RendererUID>
	{
		std::size_t operator()(const Povox::RendererUID& ruid) const
		{
			return hash<uint64_t>()((uint64_t)ruid);
		}
	};

}
