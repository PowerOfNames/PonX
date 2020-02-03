#pragma once

namespace Povox {

	typedef enum class MouseCode : uint16_t
	{
		Unknown				= -1,

		Button0				= 0,
		Button1				= 1,
		Button2				= 2,
		Button3				= 3,
		Button4				= 4,
		Button5				= 5,
		Button6				= 6,
		Button7				= 7,

		ButtonLeft			= Button0,
		ButtonRight			= Button1,
		ButtonLast			= Button7,
		ButtonMiddle		= Button2

	} Mouse;

	inline std::ostream& operator<<(std::ostream& os, MouseCode mouseCode)
	{
		os << static_cast<int32_t>(mouseCode);
		return os;
	}
}


// FRmo glfw3.h
#define PX_MOUSE_BUTTON_1         ::Povox::MouseCode::Button0
#define PX_MOUSE_BUTTON_2         ::Povox::MouseCode::Button1
#define PX_MOUSE_BUTTON_3         ::Povox::MouseCode::Button2
#define PX_MOUSE_BUTTON_4         ::Povox::MouseCode::Button3
#define PX_MOUSE_BUTTON_5         ::Povox::MouseCode::Button4
#define PX_MOUSE_BUTTON_6         ::Povox::MouseCode::Button5
#define PX_MOUSE_BUTTON_7         ::Povox::MouseCode::Button6
#define PX_MOUSE_BUTTON_8         ::Povox::MouseCode::Button7
#define PX_MOUSE_BUTTON_LEFT      ::Povox::MouseCode::ButtonLeft
#define PX_MOUSE_BUTTON_RIGHT     ::Povox::MouseCode::ButtonRight
#define PX_MOUSE_BUTTON_LAST      ::Povox::MouseCode::ButtonLast
#define PX_MOUSE_BUTTON_MIDDLE    ::Povox::MouseCode::ButtonMiddle
