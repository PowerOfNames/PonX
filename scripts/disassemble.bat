@echo off
pushd %~dp0\..\
call Povox\vendor\VulkanSDK\Bin\spirv-dis.exe Povosom\assets\cache\shader\vulkan\Texture.glsl.cached_vulkan.vert -o Povosom\assets\cache\shader\vulkan\Texture.glsl.cached_vulkan.txt
popd
pause
