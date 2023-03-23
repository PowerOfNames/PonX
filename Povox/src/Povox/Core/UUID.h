#pragma once

#include <xhash>

namespace Povox {

	/**
	 * Used for Entities in the Entity Component System. Use [RendererUID] 
	 */
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID& uuid) = default;

		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};
}


namespace std {

	template<>
	struct hash<Povox::UUID>
	{
		std::size_t operator()(const Povox::UUID& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};

}
