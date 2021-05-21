#pragma once

#include <memory>

#ifdef _WIN32
	#ifdef _WIN64
		#define PX_PLATFORM_WINDOWS
	#else 
		#error "x86 builds are not supported!"
	#endif
#elif defined(__APPLE__) || defined (__MAC__)
	#include <TargetConditionals.h>
		/*TARGET_OS_MAC exists on all the platforms.
		 * so we must check all of them (in this order)
		 * to ensure we are running on Mac and not some
		 * other apple platform
		 */
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator not supported!"
	#elif TARGET_OS_PHONE == 1
		#define PX_PLATFORM_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC
		#define PX_PLATFORM_MACOS
		#error "MACOS is not supported!"
	#else
		#error "Unknown apple platform!"
	#endif
/*We also have to check __ANDROID__ before __linux__
 * since android is based on the linux kernel,
 * it has __linux__ defined*/
#elif defined(__ANDROID__)
	#define PX_PLATFORM_ANDROID
	#error "Android is not supported!"
#elif defined (__linux__)
	#define PX_PLATFORM_LINUX
	#error "Linux is not supported!"
#else
	// unknown compiler/platform
	#error "Unknown platform!"
#endif


// DLL support
#ifdef PX_PLATFORM_WINDOWS
	#if PX_DYNAMIC_LINK
		#ifdef PX_BUILD_DLL
				#define POVOX_API __declspec(dllexport)
		#else
				#define POVOX_API __declspec(dllimport)
		#endif
	#else
		#define POVOX_API
	#endif
#else
	#error "Povox only supports windows!"
#endif

#ifdef PX_DEBUG
	#define PX_ENABLE_ASSERT
#endif

#ifdef PX_ENABLE_ASSERT
	#define PX_ASSERT(x, ...) { if(!(x)) { PX_ERROR("Assertion fails: {0}", __VA_ARGS__); __debugbreak(); } }
	#define PX_CORE_ASSERT(x, ...) { if(!(x)) { PX_CORE_ERROR("Assertion fails: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define PX_ASSERT(x, ...)
	#define PX_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define PX_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) {return this->fn(std::forward<decltype(args)>(args)...); }

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