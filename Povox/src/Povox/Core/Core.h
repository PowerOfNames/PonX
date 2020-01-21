#pragma once

#include <memory>


#ifdef PX_PLATFORM_WINDOWS
	#ifdef PX_BUILD_DLL
		#define POVOX_API __declspec(dllexport)
	#else 
		#define POVOX_API __declspec(dllimport)
	#endif
#else
	#error Povox only supports widows!
#endif

#ifdef PX_DEBUG
	#define PX_ENABLE_ASSERT
#endif

#ifdef PX_ENABLE_ASSERT
	#define PX_ASSERT(x, ...) {if(!x) { PX_ERROR("Assertion fails: {0}", __VA_ARGS__); __debugbreak();} }
	#define PX_CORE_ASSERT(x, ...) {if(!x) { PX_CORE_ERROR("Assertion fails: {0}", __VA_ARGS__); __debugbreak();} }
#else
	#define PX_ASSERT(x, ...)
	#define PX_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define PX_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

namespace Povox {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}