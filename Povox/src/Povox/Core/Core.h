#pragma once


#ifdef PX_PLATFORM_WINDOWS
	#ifdef PX_BUILD_DLL
		#define POVOX_API __declspec(dllexport)
	#else 
		#define POVOX_API __declspec(dllimport)
	#endif
#else
	#error Povox only supports widows!
#endif


#ifdef PX_ENABLE_ASSERT
	#define PX_ASSERT(x, ...) {if(!x) { PX_ERROR("Assertion fails: {0}", __VA_ARGS__); __debugbreak();} }
	#define PX_CORE_ASSERT(x, ...) {if(!x) { PX_CORE_ERROR("Assertion fails: {0}", __VA_ARGS__); __debugbreak();} }
#else
	#define PX_ASSERT(x, ...)
	#define PX_CORE_ASSERT(x, ...)
#endif


#define BIT(x) (1 << x)