#pragma once

namespace Povox {

	/**
	 * This class helps with Timer statistics. It takes deltaTime values (in ms) via AddTimestamp in and is able to return a variety of statistics.
	 */	
	class TimerStatistics
	{
	public:
		TimerStatistics();
		~TimerStatistics() = default;
						
		void AddTimestamp(double deltaTime);
		void Reset();

		inline double GetAverage() const { return m_Average; }
		inline double GetMax() const { return m_Max; }

		inline double GetTimespanAverage() const { return m_TimespanAverage; }
		inline double GetTimespanMax() const { return m_TimespanMax; }

		inline void SetTimespan(float seconds) { m_TimespanLength = seconds; }

	private:
		void CalculateTimespanAverage();
	private:
		double m_Sum				= 0.0;
		uint32_t m_TotalCount		= 0;
		double m_Average			= 0.0;
		double m_Max				= 0.0;

		float m_TimespanLength		= 1.0;
		uint64_t m_TimestampCount	= 0;
		double m_TimespanAverage	= 0.0;
		double m_TimespanMax		= 0.0;
	};	

}
