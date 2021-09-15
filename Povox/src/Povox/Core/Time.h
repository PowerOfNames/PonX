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
			auto currentTime = std::chrono::high_resolution_clock::now();
			m_FrameDuration = currentTime - m_Start;

			*m_DeltaTime = m_FrameDuration.count();
		}

		inline float GetDeltaTInMS() const { return m_FrameDuration.count() * 1000.0f; }
		inline float GetDeltaTInSec() const { return m_FrameDuration.count(); }

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_Start;
		std::chrono::duration<float> m_FrameDuration;

		float* m_DeltaTime;
	};

	class Timer
	{
	public:
		Timer()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		~Timer() = default;

		float ElapsedMilliseconds()
		{
			return ((std::chrono::duration<float>)(std::chrono::high_resolution_clock::now() - m_Start)).count() * 1000.0;
		}

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_Start;
	};
}
