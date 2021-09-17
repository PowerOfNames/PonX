# Povox
PowerOfNames Voxel-Engine

This is a openGL learning project of mine with the goal to create my own little voxel based engine.
I follow the openGl/Game-engine series from TheCherno on youtube and this engine will be fairly similiar to his "Hazel"-engine, up to the point I concentrate on making the voxel-renderer.

I'm really grateful for his effords making such great work on youtube and showing people how to come up with such a complex and creative topic.



How to install:

First, git clone --recursive https://github.com/PowerOfNames/Povox/tree/vulkanImpl <- for the current dev-branch

I don't provide the vulkan-files (too big) so please download the most recent VulkanSDK, if you don't have them already and install them via installer from https://vulkan.lunarg.com/sdk/home
I used just the most recent windows SDK installer.

After that, I copied everything within the version file ("wherever you've installed it"/"version"/...) into its own VulkanSDK directory inside Povox/vendor
You can install Vulkan directly into it, but care for the parent-version folder.

After that, create a system environment variable called VulkanSDK_Povox! (or whatever you like)

Paste the name you've choosen into Dependencies.lua and replace the first line Vulkan_SDK = os.getenv(your name).
Alternatively, you can replace the whole os.getenv with "%{wks.location}Povox/vendor/VulkanSDK"


Install the latest! premake5 version from https://premake.github.io/download and place the .exe file into ./vendor/premake/bin in the root directory.
You may need to create that.


After that, run the win-genProjects.bat inside scripts and you should be able to run the engine!