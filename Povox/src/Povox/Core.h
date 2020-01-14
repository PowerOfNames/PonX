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