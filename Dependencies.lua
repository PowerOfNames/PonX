

-- Povox dependencies


Vulkan_SDK = "%{wks.location}/Povox/vendor/VulkanSDK"
-- Vulkan_SDK = os.getenv("VulkanSDK_Povox")


IncludeDir = {}
IncludeDir["GLFW"]			= "%{wks.location}/Povox/vendor/GLFW/include"
IncludeDir["Glad"]			= "%{wks.location}/Povox/vendor/Glad/include"
IncludeDir["ImGui"]			= "%{wks.location}/Povox/vendor/ImGui"
IncludeDir["glm"]			= "%{wks.location}/Povox/vendor/glm"
IncludeDir["stb_image"]		= "%{wks.location}/Povox/vendor/stb_image"
IncludeDir["entt"]			= "%{wks.location}/Povox/vendor/entt/include"
IncludeDir["yaml_cpp"]		= "%{wks.location}/Povox/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"]		= "%{wks.location}/Povox/vendor/ImGuizmo"
IncludeDir["VulkanSDK"]		= "%{Vulkan_SDK}/Include"
IncludeDir["SPIRV-cross"]	= "%{wks.location}/Povox/vendor/SPIRV-cross"
IncludeDir["shaderc"]		= "%{wks.location}/Povox/vendor/shaderc"


LibraryDir = {}
LibraryDir["VulkanSDK"]				= "%{Vulkan_SDK}/Lib"
LibraryDir["VulkanSDK_Debug"]		= "%{Vulkan_SDK}/Lib"
LibraryDir["VulkanSDK_DebugDLL"]	= "%{Vulkan_SDK}/Bin"


Library = {}
Library["Vulkan"]					= "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"]				= "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"]			= "%{LibraryDir.VulkanSDK_Debug}/shaderc_sharedd.lib"
Library["SPIRV_cross_Debug"]		= "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-cored.lib"
Library["SPIRV_cross_glsl_Debug"]	= "%{LibraryDir.VulkanSDK_Debug}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"]		= "%{LibraryDir.VulkanSDK_Debug}/spirv-Toolsd.lib"

Library["ShaderC_Release"]			= "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_cross_Release"]		= "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_cross_glsl_Release"]	= "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"




