include "Elysium/elysiumlink.lua"
include "Elysium/vendor/imgui_suite/imgui_dependencies.lua"
include "vendor/opencv_lib/opencv4link.lua"

project "SVisualizer"
	kind "ConsoleApp"

	language "C++"
	cppdialect "C++17"

	staticruntime "on"

	targetdir ("%{wks.location}/Binaries/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/Intermediates/" .. outputdir .. "/%{prj.name}")

	pchheader "svis_pch.h"
	pchsource "src/svis_pch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"../Content/**"
	}

	includedirs
	{
		"src",

		"%{ImGui_IncludeDir.ImGui}",
		"%{ImGui_IncludeDir.ImGuizmo}",
		"%{ImGui_IncludeDir.ImNodes}",
		"%{ImGui_IncludeDir.ImTextEditor}",
		
		"%{IncludeDir.entt}",
		"%{IncludeDir.IconFontCppHeaders}",
		"%{IncludeDir.stduuid}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.spd_log}"
	}

	postbuildcommands
	{
		"{COPY} " .. '"' .. "%{wks.location}/SVisualizer/imgui.ini" .. '"' .. " %{cfg.targetdir}"
	}

	LinkElysium()
	LinkOpenCV4()

	filter "system:windows"
		systemversion "latest"
	filter "configurations:Debug"
		defines "ELYSIUM_DEBUG"
		symbols "On"
	filter "configurations:Release"
		defines "ELYSIUM_RELEASE"
		optimize "On"
	filter "configurations:Dist"
		defines "ELYSIUM_DIST"
		optimize "Full"