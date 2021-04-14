#pragma once

#include "Povox/Core/KeyCodes.h"
#include "Povox/Core/MouseCodes.h"

#include <glm/glm.hpp>

namespace Povox {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);

		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
