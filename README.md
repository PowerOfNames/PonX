# Povox
PowerOfNames Voxel-Engine

This project started as an OpenGL learning tool for me with the goal to create my own little engine.
I followed the OpenGl (https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2) / Game-engine (https://www.youtube.com/watch?v=JxIZbV_XjAs&list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT) series from TheCherno (https://www.youtube.com/channel/UCQ-W1KE9EYfdxhL6S4twUNw) on Youtube and this engine will seem fairly similiar to his "Hazel"-engine for some part of the code-base.

During the process, I became really passionate about graphics rendering (although I still didn't quite had the time and resources yet to experiment around... and the engine is under construction now since a loong time :D )
As a fellow computer scientist with not much experience in programming in general as I started the engine, this became one of the biggest learning resources to me and I still learn every day.

In the future, I hope to expand the project to not only make little games, but also do some simulations, 3D rendering, volume rendering and more.



***

## Getting Started

<ins>**1. Downloading the repository:**</ins>

Prerequisites: Install the latest VulkanSDK (https://sdk.lunarg.com/sdk/download/1.3.236.0/windows/VulkanSDK-1.3.236.0-Installer.exe)

First clone the repository with `git clone --recursive https://github.com/PowerOfNames/Povox`.

If the repository was cloned non-recursively previously, use `git submodule update --init` to clone the necessary submodules.

<ins>**2. Configuration and project build:**</ins>

After everything up until now has been done, check that your VulkanSDK env-var is named Vulkan_SDK, or replace in the Dependencies.lua under Vulkan_SDK = os.getenv("Vulkan_SDK") 'Vulkan_SDK' whith whatever your env-var is called.
Next, navigate into scripts and run 'win-genProjects.bat' to generate the solution files.

Technically, the application should be runable in Debug-mode now. (The Release and Dist modes I need to reconfigure, there is something messed up in my premake files)

***

## Content

The engine is currently under heavy construction, as I move away from OpenGL and towards Vulkan. 



***

## Contact

Feel free to contact me via Discord anytime, - PowerOfNames