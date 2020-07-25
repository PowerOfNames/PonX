#pragma once

#include "Povox/Renderer/PerspectiveCameraController.h"
#include "Povox/Renderer/Scene/Lighting.h"

namespace Povox {

	class RayTracer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(PerspectiveCamera& camera, Light& lightsource);
		static void EndScene();

		static void Trace(PerspectiveCameraController& cameraController);
	};
}