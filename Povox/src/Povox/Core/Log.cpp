#include "pxpch.h"

#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>


namespace Povox {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Init()
	{
		// Sets the pattern for the logging in the form of: 'color in range' of ['time'] 'logger name': 'message''end of color range'
		// Color depends on severity of the message
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("POVOX");
		s_CoreLogger->set_level(spdlog::level::level_enum::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::level_enum::trace);
	}
}
