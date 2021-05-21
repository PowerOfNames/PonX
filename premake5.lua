workspace "Povox"
	architecture "x86_64"
	startproject "Povosom"

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
IncludeDir["Vulkan"] = ("Povox/vendor/Vulkan/Include")
IncludeDir["ImGui"] = ("Povox/vendor/ImGui")
IncludeDir["glm"] = ("Povox/vendor/glm")
IncludeDir["stb_image"] = ("Povox/vendor/stb_image")
IncludeDir["entt"] = ("Povox/vendor/entt/include")
IncludeDir["yaml_cpp"] = ("Povox/vendor/yaml-cpp/include")
IncludeDir["ImGuizmo"] = ("Povox/vendor/ImGuizmo")



group "Dependencies"
	include "Povox/vendor/GLFW"
	include "Povox/vendor/Glad"
	include "Povox/vendor/ImGui"
	include "Povox/vendor/yaml-cpp"

group ""

project "Povox"
	location "Povox"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"


	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pxpch.h"
	pchsource "Povox/src/pxpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",

		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.Vulkan}"
	}

	links
	{
		"GLFW",
		"Glad",
		"ImGui",
		"yaml-cpp",
		"opengl32.lib",
		"vulkan-1.lib"
	}
	
	libdirs
	{
		"%{prj.name}/vendor/Vulkan/Lib"
	}

	filter "files:Povox/vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }


	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PX_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "PX_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "PX_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "PX_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"



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
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}"
	}

	links
	{
		"Povox"
	}

	filter "system:windows"
		systemversion "latest"


	filter "configurations:Debug"
		defines "PX_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "PX_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "PX_DIST"
		runtime "Release"
		optimize "on"


project "Povosom"
	location "Povosom"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"



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
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"Povox"
	}

	filter "system:windows"
		systemversion "latest"


	filter "configurations:Debug"
		defines "PX_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "PX_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "PX_DIST"
		runtime "Release"
		optimize "on"