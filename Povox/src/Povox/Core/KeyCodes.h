#pragma once


namespace Povox {

	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Unknown					= -1,

		Space					= 32,
		Apostrophe				= 39,			/* ' */
		Comma					= 44,			/* , */
		Minus					= 45,			/* - */
		Period					= 46,			/* . */
		Slash					= 47,			/* / */
		D0						= 48,
		D1						= 49,
		D2						= 50,
		D3						= 51,
		D4						= 52,
		D5						= 53,
		D6						= 54,
		D7						= 55,
		D8						= 56,
		D9						= 57,
		Semicolon				= 59,			/* ; */
		Equal					= 61,			/* = */
		A						= 65,
		B						= 66,
		C						= 67,
		D						= 68,
		E						= 69,
		F						= 70,
		G						= 71,
		H						= 72,
		I						= 73,
		J						= 74,
		K						= 75,
		L						= 76,
		M						= 77,
		N						= 78,
		O						= 79,
		P						= 80,
		Q						= 81,
		R						= 82,
		S						= 83,
		T						= 84,
		U						= 85,
		V						= 86,
		W						= 87,
		X						= 88,
		Y						= 89,
		Z						= 90,
		LeftBracket				= 91,			/* [ */
		Backslash				= 92,			/* / */
		RightBracket			= 93,			/* ] */
		GraveAccent				= 96,			/* ` */
		World1					= 161,			/* non-US #1 */
		World2					= 162,			/* non-US #2 */

		Escape					= 256,
		Enter					= 257,
		Tab						= 258,
		Backspace				= 259,
		Insert					= 260,
		Delete					= 261,
		Right					= 262,
		Left					= 263,
		Down					= 264,
		Up						= 265,
		PageUp					= 266,
		PageDown				= 267,
		Home					= 268,
		End						= 269,
		CapsLock				= 280,
		ScrollLock				= 281,
		NumLock					= 282,
		PrintScreen				= 283,
		Pause					= 284,
		F1						= 290,
		F2						= 291,
		F3						= 292,
		F4						= 293,
		F5						= 294,
		F6						= 295,
		F7						= 296,
		F8						= 297,
		F9						= 298,
		F10						= 299,
		F11						= 300,
		F12						= 301,
		F13						= 302,
		F14						= 303,
		F15						= 304,
		F16						= 305,
		F17						= 306,
		F18						= 307,
		F19						= 308,
		F20						= 309,
		F21						= 310,
		F22						= 311,
		F23						= 312,
		F24						= 313,
		F25						= 314,
		Numpad0					= 320,
		Numpad1					= 321,
		Numpad2					= 322,
		Numpad3					= 323,
		Numpad4					= 324,
		Numpad5					= 325,
		Numpad6					= 326,
		Numpad7					= 327,
		Numpad8					= 328,
		Numpad9					= 329,
		NumpadDecimal			= 330,
		NumpadDivide			= 331,
		NumpadMultiply			= 332,
		NumpadSubtract			= 333,
		NumpadAdd				= 334,
		NumpadEnter				= 335,
		NumpadEqual				= 336,
		LeftShift				= 340,
		LeftControl				= 341,
		LeftAlt					= 342,
		LeftSuper				= 343,
		RightShift				= 344,
		RightControl			= 345,
		RightAlt				= 346,
		RightSuper				= 347,
		Menu					= 348,

		KeyLast					= Menu,
	} Keys;

	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}
}

// Keeping those macros for backward-compatibility (all taken from glfw3.h)

#define PX_KEY_SPACE           ::Povox::Keys::Space
#define PX_KEY_APOSTROPHE      ::Povox::Keys::Apostrophe    /* ' */
#define PX_KEY_COMMA           ::Povox::Keys::Comma         /* , */
#define PX_KEY_MINUS           ::Povox::Keys::Minus         /* - */
#define PX_KEY_PERIOD          ::Povox::Keys::Period        /* . */
#define PX_KEY_SLASH           ::Povox::Keys::Slash         /* / */
#define PX_KEY_0               ::Povox::Keys::D0
#define PX_KEY_1               ::Povox::Keys::D1
#define PX_KEY_2               ::Povox::Keys::D2
#define PX_KEY_3               ::Povox::Keys::D3
#define PX_KEY_4               ::Povox::Keys::D4
#define PX_KEY_5               ::Povox::Keys::D5
#define PX_KEY_6               ::Povox::Keys::D6
#define PX_KEY_7               ::Povox::Keys::D7
#define PX_KEY_8               ::Povox::Keys::D8
#define PX_KEY_9               ::Povox::Keys::D9
#define PX_KEY_SEMICOLON       ::Povox::Keys::Semicolon     /* ; */
#define PX_KEY_EQUAL           ::Povox::Keys::Equal         /* = */
#define PX_KEY_A               ::Povox::Keys::A
#define PX_KEY_B               ::Povox::Keys::B
#define PX_KEY_C               ::Povox::Keys::C
#define PX_KEY_D               ::Povox::Keys::D
#define PX_KEY_E               ::Povox::Keys::E
#define PX_KEY_F               ::Povox::Keys::F
#define PX_KEY_G               ::Povox::Keys::G
#define PX_KEY_H               ::Povox::Keys::H
#define PX_KEY_I               ::Povox::Keys::I
#define PX_KEY_J               ::Povox::Keys::J
#define PX_KEY_K               ::Povox::Keys::K
#define PX_KEY_L               ::Povox::Keys::L
#define PX_KEY_M               ::Povox::Keys::M
#define PX_KEY_N               ::Povox::Keys::N
#define PX_KEY_O               ::Povox::Keys::O
#define PX_KEY_P               ::Povox::Keys::P
#define PX_KEY_Q               ::Povox::Keys::Q
#define PX_KEY_R               ::Povox::Keys::R
#define PX_KEY_S               ::Povox::Keys::S
#define PX_KEY_T               ::Povox::Keys::T
#define PX_KEY_U               ::Povox::Keys::U
#define PX_KEY_V               ::Povox::Keys::V
#define PX_KEY_W               ::Povox::Keys::W
#define PX_KEY_X               ::Povox::Keys::X
#define PX_KEY_Y               ::Povox::Keys::Y
#define PX_KEY_Z               ::Povox::Keys::Z
#define PX_KEY_LEFT_BRACKET    ::Povox::Keys::LeftBracket   /* [ */
#define PX_KEY_BACKSLASH       ::Povox::Keys::Backslash     /* \ */
#define PX_KEY_RIGHT_BRACKET   ::Povox::Keys::RightBracket  /* ] */
#define PX_KEY_GRAVE_ACCENT    ::Povox::Keys::GraveAccent   /* ` */
#define PX_KEY_WORLD_1         ::Povox::Keys::World1        /* non-US #1 */
#define PX_KEY_WORLD_2         ::Povox::Keys::World2        /* non-US #2 */
										   
/* Function keys */						   
#define PX_KEY_ESCAPE          ::Povox::Keys::Escape
#define PX_KEY_ENTER           ::Povox::Keys::Enter
#define PX_KEY_TAB             ::Povox::Keys::Tab
#define PX_KEY_BACKSPACE       ::Povox::Keys::Backspace
#define PX_KEY_INSERT          ::Povox::Keys::Insert
#define PX_KEY_DELETE          ::Povox::Keys::Delete
#define PX_KEY_RIGHT           ::Povox::Keys::Right
#define PX_KEY_LEFT            ::Povox::Keys::Left
#define PX_KEY_DOWN            ::Povox::Keys::Down
#define PX_KEY_UP              ::Povox::Keys::Up
#define PX_KEY_PAGE_UP         ::Povox::Keys::PageUp
#define PX_KEY_PAGE_DOWN       ::Povox::Keys::PageDown
#define PX_KEY_HOME            ::Povox::Keys::Home
#define PX_KEY_END             ::Povox::Keys::End
#define PX_KEY_CAPS_LOCK       ::Povox::Keys::CapsLock
#define PX_KEY_SCROLL_LOCK     ::Povox::Keys::ScrollLock
#define PX_KEY_NUM_LOCK        ::Povox::Keys::NumLock
#define PX_KEY_PRINT_SCREEN    ::Povox::Keys::PrintScreen
#define PX_KEY_PAUSE           ::Povox::Keys::Pause
#define PX_KEY_F1              ::Povox::Keys::F1
#define PX_KEY_F2              ::Povox::Keys::F2
#define PX_KEY_F3              ::Povox::Keys::F3
#define PX_KEY_F4              ::Povox::Keys::F4
#define PX_KEY_F5              ::Povox::Keys::F5
#define PX_KEY_F6              ::Povox::Keys::F6
#define PX_KEY_F7              ::Povox::Keys::F7
#define PX_KEY_F8              ::Povox::Keys::F8
#define PX_KEY_F9              ::Povox::Keys::F9
#define PX_KEY_F10             ::Povox::Keys::F10
#define PX_KEY_F11             ::Povox::Keys::F11
#define PX_KEY_F12             ::Povox::Keys::F12
#define PX_KEY_F13             ::Povox::Keys::F13
#define PX_KEY_F14             ::Povox::Keys::F14
#define PX_KEY_F15             ::Povox::Keys::F15
#define PX_KEY_F16             ::Povox::Keys::F16
#define PX_KEY_F17             ::Povox::Keys::F17
#define PX_KEY_F18             ::Povox::Keys::F18
#define PX_KEY_F19             ::Povox::Keys::F19
#define PX_KEY_F20             ::Povox::Keys::F20
#define PX_KEY_F21             ::Povox::Keys::F21
#define PX_KEY_F22             ::Povox::Keys::F22
#define PX_KEY_F23             ::Povox::Keys::F23
#define PX_KEY_F24             ::Povox::Keys::F24
#define PX_KEY_F25             ::Povox::Keys::F25
										   
/* Keypad */							   
#define PX_KEY_KP_0            ::Povox::Keys::Numpad0			
#define PX_KEY_KP_1            ::Povox::Keys::Numpad1			
#define PX_KEY_KP_2            ::Povox::Keys::Numpad2			
#define PX_KEY_KP_3            ::Povox::Keys::Numpad3			
#define PX_KEY_KP_4            ::Povox::Keys::Numpad4			
#define PX_KEY_KP_5            ::Povox::Keys::Numpad5			
#define PX_KEY_KP_6            ::Povox::Keys::Numpad6			
#define PX_KEY_KP_7            ::Povox::Keys::Numpad7			
#define PX_KEY_KP_8            ::Povox::Keys::Numpad8			
#define PX_KEY_KP_9            ::Povox::Keys::Numpad9			
#define PX_KEY_KP_DECIMAL      ::Povox::Keys::NumpadDecimal	
#define PX_KEY_KP_DIVIDE       ::Povox::Keys::NumpadDivide	
#define PX_KEY_KP_MULTIPLY     ::Povox::Keys::NumpadMultiply	
#define PX_KEY_KP_SUBTRACT     ::Povox::Keys::NumpadSubtract	
#define PX_KEY_KP_ADD          ::Povox::Keys::NumpadAdd		
#define PX_KEY_KP_ENTER        ::Povox::Keys::NumpadEnter		
#define PX_KEY_KP_EQUAL        ::Povox::Keys::NumpadEqual		
										   
#define PX_KEY_LEFT_SHIFT      ::Povox::Keys::LeftShift
#define PX_KEY_LEFT_CONTROL    ::Povox::Keys::LeftControl
#define PX_KEY_LEFT_ALT        ::Povox::Keys::LeftAlt
#define PX_KEY_LEFT_SUPER      ::Povox::Keys::LeftSuper
#define PX_KEY_RIGHT_SHIFT     ::Povox::Keys::RightShift
#define PX_KEY_RIGHT_CONTROL   ::Povox::Keys::RightControl
#define PX_KEY_RIGHT_ALT       ::Povox::Keys::RightAlt
#define PX_KEY_RIGHT_SUPER     ::Povox::Keys::RightSuper
#define PX_KEY_MENU            ::Povox::Keys::Menu
