#pragma once

#include "Povox/Core/Core.h"
#include "Povox/Core/KeyCodes.h"
#include "Povox/Core/MouseButtonCodes.h"

#include "Povox/Core/Log.h"

namespace Povox {

	struct KeyAlternative
	{
		std::variant<KeyCode, MouseCode> value;

		KeyAlternative() = default;
		KeyAlternative(KeyCode key) { value = key; }
		KeyAlternative(MouseCode mouseButton) { value = mouseButton; }
	};

	class POVOX_API Input
	{
	public:
		inline static bool IsInputPressed(const std::string& name)
		{
			if (s_Instance->m_KeyMapping.count(name) != 0)
			{
				return s_Instance->IsInputPressed(s_Instance->m_KeyMapping[name]);
			}
			PX_CORE_WARN("Key: '{0}' is not mapped!", name);
			return false;
		}

		inline static bool IsInputPressed(KeyAlternative input)
		{
			if (std::holds_alternative<KeyCode>(input.value))
			{
				return s_Instance->IsKeyPressedImpl((int)std::get<KeyCode>(input.value));
			}
			if (std::holds_alternative<MouseCode>(input.value))
			{
				return s_Instance->IsMouseButtonPressedImpl((int)std::get<MouseCode>(input.value));
			}
			// Should never get down here!
			PX_CORE_WARN("Unknown input type !");
			return false;
		}

		inline static bool IsKeyPressed(int keycode) { return s_Instance->IsKeyPressedImpl(keycode); }
		inline static bool IsKeyPressed(Keys key) { return s_Instance->IsKeyPressedImpl((int)key); }

		inline static bool IsMouseButtonPressed(int button) { return s_Instance->IsMouseButtonPressedImpl(button); }
		inline static bool IsMouseButtonPressed(Mouse button) { return s_Instance->IsMouseButtonPressedImpl((int)button); }

		inline static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
		inline static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
		inline static float GetMouseY() { return s_Instance->GetMouseYImpl(); }

		inline static void Remap(const std::string& name, KeyAlternative input)	{ s_Instance->m_KeyMapping[name] = input; }

	protected:
		virtual bool IsKeyPressedImpl(int keycode) = 0;

		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual std::pair<float, float> GetMousePositionImpl() = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;

	private:
		std::unordered_map<std::string, KeyAlternative> m_KeyMapping;
		
		static Scope<Input> s_Instance;
	};
}
