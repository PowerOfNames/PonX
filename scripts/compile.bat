@echo off
pushd %~dp0\..\
call Povox\vendor\VulkanSDK\Bin\glslc.exe Povox\src\Platform\Vulkan\shader.vert -o Sandbox\assets\shaders\vert.spv
call Povox\vendor\VulkanSDK\Bin\glslc.exe Povox\src\Platform\Vulkan\shader.frag -o Sandbox\assets\shaders\frag.spv
popd
pause
