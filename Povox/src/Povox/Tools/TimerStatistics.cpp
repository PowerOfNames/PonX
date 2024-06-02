#include "pxpch.h"
#include "Povox/Tools/TimerStatistics.h"

namespace Povox {

	TimerStatistics::TimerStatistics() {}

	//deltaTime in ms
	void TimerStatistics::AddTimestamp(double deltaTimeMS)
	{
		m_Sum += deltaTimeMS;
		m_TimestampCount++;
		m_Average = m_Sum / m_TimestampCount;

		if (deltaTimeMS > m_Max)
			m_Max = deltaTimeMS;

		CalculateTimespanAverage();
	}

	void TimerStatistics::CalculateTimespanAverage()
	{

	}

	void TimerStatistics::Reset()
	{
		m_Sum				= 0.0;
		m_TotalCount		= 0;
		m_Average			= 0.0;
		m_Max				= 0.0;

		m_TimespanAverage	= 0.0;
		m_TimestampCount	= 0;
		m_TimespanMax		= 0.0;
	}
}
