project "xxHash"
	kind "StaticLib"
	language "C"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{		
		"xxHash.c"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "On"

		files
		{
		}

		defines 
		{ 
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
