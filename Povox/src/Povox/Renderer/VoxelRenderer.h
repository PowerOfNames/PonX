#pragma once
#include "Povox/Renderer/PerspectiveCamera.h"

#include "Povox/Renderer/Buffer.h"

#include <glm/glm.hpp>

namespace Povox {

	class VoxelRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(PerspectiveCamera& camera);
		static void EndScene();

		static void Flush();

		static void OnWindowResize(uint32_t width, uint32_t height);
		
	// Primitives
		static void DrawCube(glm::vec3 position, float scale, glm::vec4 color);
	};

}
