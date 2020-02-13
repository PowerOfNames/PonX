#pragma once
#include "Povox/Renderer/PerspectiveCamera.h"

#include <glm/glm.hpp>

namespace Povox {

	class VoxelRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(PerspectiveCamera& camera);
		static void EndScene();

	// Primitives
		static void DrawCube(glm::vec3 position, glm::vec3 size, glm::vec4 color);
	};

}
