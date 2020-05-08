#pragma once

#include "Povox/Renderer/PerspectiveCamera.h"

namespace Povox {

	class RayTracer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(PerspectiveCamera& camera);
		static void EndScene();

		static void Trace(PerspectiveCamera& camera);
	};
}