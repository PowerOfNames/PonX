@echo off
pushd %~dp0\..\
call Povox\vendor\Vulkan\Bin\glslc.exe Povox\src\Platform\Vulkan\shader.vert -o Povox\src\Platform\Vulkan\vert.spv
call Povox\vendor\Vulkan\Bin\glslc.exe Povox\src\Platform\Vulkan\shader.frag -o Povox\src\Platform\Vulkan\frag.spv
popd
pause