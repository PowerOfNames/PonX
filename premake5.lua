workspace "Povox"
	architecture "x86_64"
	startproject "Sandbox"

	configurations 
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder
IncludeDir = {}
IncludeDir["GLFW"] = ("Povox/vendor/GLFW/include")
IncludeDir["Glad"] = ("Povox/vendor/Glad/include")
IncludeDir["ImGui"] = ("Povox/vendor/ImGui")
IncludeDir["glm"] = ("Povox/vendor/glm")

group "Dependencies"
	include "Povox/vendor/GLFW"
	include "Povox/vendor/Glad"
	include "Povox/vendor/ImGui"

group ""

project "Povox"
	location "Povox"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"


	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pxpch.h"
	pchsource "Povox/src/pxpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}



	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PX_PLATFORM_WINDOWS",
			"PX_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			("{COPY} %{cfg.buildtarget.relpath} \"../bin/" .. outputdir .. "/Sandbox/\"")
		}

	filter "configurations:Debug"
		defines "PX_DEBUG"
		runtime "Debug"
		symbols "On"
		
	filter "configurations:Release"
		defines "PX_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "PX_DIST"
		runtime "Release"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"



	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Povox/src",
		"Povox/vendor/spdlog/include",
		"Povox/vendor",
		"%{IncludeDir.glm}"
	}

	links
	{
		"Povox"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PX_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "PX_DEBUG"
		runtime "Debug"
		symbols "On"
		
	filter "configurations:Release"
		defines "PX_RELEASE"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		defines "PX_DIST"
		runtime "Release"
		optimize "On"