#pragma once
#include <iostream>
#include <string>
#include <chrono>

namespace Povox {

	class MyTimestep
	{
	public:
		MyTimestep(float* value) 
			: m_DeltaTime(value), m_FrameDuration((std::chrono::duration<float>)0.0f)
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}
		~MyTimestep()
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



	template<typename Fn>
	class Timer
	{
	public:
		Timer(const char* name, Fn&& func)
			: m_Name(name), m_Func(func), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::high_resolution_clock::now();
		}

		~Timer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimePoint = std::chrono::high_resolution_clock::now();

			long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
			long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();

			float duration = (end - start) * 0.001;
			m_Func({ m_Name, duration });
			m_Stopped = true;
		}

	private:
		const char* m_Name;
		Fn m_Func;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped;
	};
}
