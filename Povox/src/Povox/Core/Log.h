#pragma once

#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Povox {

	class POVOX_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger;  }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger;  }
	private:
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
	};

}


// Core log macros
#define PX_CORE_TRACE(...)   ::Povox::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PX_CORE_INFO(...)    ::Povox::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PX_CORE_WARN(...)    ::Povox::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PX_CORE_ERROR(...)   ::Povox::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PX_CORE_CRITICAL(...)   ::Povox::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define PX_TRACE(...)        ::Povox::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PX_INFO(...)         ::Povox::Log::GetClientLogger()->info(__VA_ARGS__)
#define PX_WARN(...)         ::Povox::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PX_ERROR(...)        ::Povox::Log::GetClientLogger()->error(__VA_ARGS__)
#define PX_CRITICAL(...)        ::Povox::Log::GetClientLogger()->critical(__VA_ARGS__)