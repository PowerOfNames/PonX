include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace "Povox"
	architecture "x86_64"
	startproject "Povoton"

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
	include "Povox/vendor/ImGui"
group ""

include "Povox"
include "Sandbox"
include "Povosom"
include "Povoton"







