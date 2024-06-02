project "Povox"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"


	targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pxpch.h"
	pchsource "src/pxpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",

		"vendor/stb_image/**.cpp",
		"vendor/stb_image/**.h",

		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
		"vendor/VMA/**.h",
		
		"vendor/SPIRV-Reflect/spirv_reflect.cpp",
		"vendor/SPIRV-Reflect/spirv_reflect.h",

		-- thanks to nepp95, who came up with this (compiling yaml here instead in its own premake.lua) cause linking it is a pain...
		"vendor/yaml-cpp/src/**.h",
		"vendor/yaml-cpp/src/**.cpp",
		"vendor/yaml-cpp/include/**.h",

		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
		"YAML_CPP_STATIC_DEFINE"
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.xxHash}",

		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.VMA}",

		"%{IncludeDir.SPIRV_Reflect}"
	}

	links
	{
		"GLFW",
		"ImGui",
		"xxHash",
		"%{Library.Vulkan}"
	}

	filter "files:vendor/ImGuizmo/**.cpp"
		flags { "NoPCH" }

	filter "files:vendor/yaml-cpp/src/**.cpp"
        flags { "NoPCH" }

	filter "files:vendor/SPIRV-Reflect/**.cpp"
        flags { "NoPCH" }
		
	filter "files:vendor/xxHash/**.c"
        flags { "NoPCH" }


	filter "system:windows"
		systemversion "latest"
		
	filter "action:vs*"
		buildoptions { "/utf-8" }		
		characterset "Unicode"

	filter "configurations:Debug"
		defines "PX_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_cross_Debug}",
			"%{Library.SPIRV_cross_glsl_Debug}"
		}

		
	filter "configurations:Release"
		defines "PX_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",			
			"%{Library.SPIRV_cross_Release}",		
			"%{Library.SPIRV_cross_glsl_Release}"
		}

	filter "configurations:Dist"
		defines "PX_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			"%{Library.ShaderC_Release}",			
			"%{Library.SPIRV_cross_Release}",		
			"%{Library.SPIRV_cross_glsl_Release}"
		}
