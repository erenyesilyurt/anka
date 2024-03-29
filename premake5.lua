workspace "AnkaEngine"
	architecture "x86_64"
	
	configurations
	{
		"Debug",
		"Release"
	}
	
	flags
	{
		"MultiProcessorCompile",
		"ShadowedVariables"
	}
	
	project "Anka"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		staticruntime "on"
		
		files
		{
			"src/**.cpp",
			"src/**.hpp",
			"src/evaluation/**.cpp",
			"src/evaluation/**.hpp",
			"src/external/Pyrrhic/tbprobe.cpp",
			"src/external/Pyrrhic/*.h"
		}
		
		
		targetdir ("bin/%{cfg.buildcfg}")
		objdir ("build_files/%{cfg.buildcfg}")
		
		
		includedirs
		{
			"src",
			"src/evaluation",
			"src/external/Pyrrhic"
		}
		
		filter "configurations:Debug"
			defines {"ANKA_DEBUG"}
			runtime "Debug"
			symbols "on"
		
		
		filter "configurations:Release"
			runtime "Release"
			optimize "Speed"
			intrinsics "on"
			flags
			{
				"LinkTimeOptimization"
			}
			
		filter "system:windows"
			defines
			{
				"_CRT_SECURE_NO_WARNINGS"
			}
			systemversion "latest"
		
		filter "system:not windows"
			links
			{
				"pthread"
			}

			buildoptions 
			{
				"-mlzcnt",
				"-mpopcnt",
				"-mbmi"
			}