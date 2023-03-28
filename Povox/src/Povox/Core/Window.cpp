#include "pxpch.h"
#include "Povox/Core/Window.h"

#ifdef PX_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Povox {

	Scope<Window> Window::Create(const WindowProps& props)
	{
#ifdef PX_PLATFORM_WINDOWS
		PX_CORE_INFO("Window::Create: Selecting WindowsWindow!");
		return CreateScope<WindowsWindow>(props);
#else
		PX_CORE_INFO("Window::Create: No window selected!");
		PX_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}
