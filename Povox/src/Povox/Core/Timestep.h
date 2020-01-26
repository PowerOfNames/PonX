#pragma once
#include <chrono>

namespace Povox {

	class Timer
	{
	public:
		Timer(float* value) 
			: m_DeltaTime(value), m_FrameDuration((std::chrono::duration<float>)0.0f)
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}
		~Timer()
		{
			m_End = std::chrono::high_resolution_clock::now();
			m_FrameDuration = m_End - m_Start;

			*m_DeltaTime = m_FrameDuration.count();
			PX_CORE_TRACE("Deltatime = {0}", *m_DeltaTime);
		}

		inline float GetDeltaTInMS() const { return m_FrameDuration.count() * 1000.0f; }
		inline float GetDeltaTInSec() const { return m_FrameDuration.count(); }

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_Start, m_End;
		std::chrono::duration<float> m_FrameDuration;

		float* m_DeltaTime;
	};
}
