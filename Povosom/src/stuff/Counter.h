#pragma once


namespace Povox {

	class Counter
	{
	public: 
		Counter(uint32_t start);
		~Counter() = default;

		inline void Increment(uint32_t amount = 1) { m_Current += amount; }
		inline uint32_t GetCurrent() { return m_Current; }

	private:
		uint32_t m_Current = 0;
	};
}
