#pragma once

namespace Povox {

	class Timestep
	{



	};

}

class Timer
{
private:
	std::chrono::time_point<std::chrono::steady_clock> m_Start, m_End;
	std::chrono::duration<float> m_Duration;

	float* m_DeltaTime; //stores the adress of the deltaTime in the application
public:
	Timer(float* value);
	~Timer();

	float Duration() { return m_Duration.count() * 1000.0f; }
};

Timer::Timer(float* value)
	: m_DeltaTime(value), m_Duration((std::chrono::duration<float>)0.0f)
{
	m_Start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer()
{
	m_End = std::chrono::high_resolution_clock::now();
	m_Duration = m_End - m_Start;

	*m_DeltaTime = m_Duration.count() * 1000.0f;
}
