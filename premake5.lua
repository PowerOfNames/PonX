include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Povox"
	architecture "x86_64"
	startproject "Povosom"

	configurations 
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "Povox/vendor/GLFW"
	include "Povox/vendor/Glad"
	include "Povox/vendor/ImGui"
	--include "Povox/vendor/yaml-cpp"
group ""

include "Povox"
include "Sandbox"
include "Povosom"







