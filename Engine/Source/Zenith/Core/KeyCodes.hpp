#pragma once

#include <ostream>
#include <cstdint>
#include <SDL3/SDL.h>

namespace Zenith
{
	typedef enum class KeyCode : uint32_t
	{
		// From SDL3 scancode values
		Space = SDL_SCANCODE_SPACE,
		Apostrophe = SDL_SCANCODE_APOSTROPHE, /* ' */
		Comma = SDL_SCANCODE_COMMA, /* , */
		Minus = SDL_SCANCODE_MINUS, /* - */
		Period = SDL_SCANCODE_PERIOD, /* . */
		Slash = SDL_SCANCODE_SLASH, /* / */

		D0 = SDL_SCANCODE_0, /* 0 */
		D1 = SDL_SCANCODE_1, /* 1 */
		D2 = SDL_SCANCODE_2, /* 2 */
		D3 = SDL_SCANCODE_3, /* 3 */
		D4 = SDL_SCANCODE_4, /* 4 */
		D5 = SDL_SCANCODE_5, /* 5 */
		D6 = SDL_SCANCODE_6, /* 6 */
		D7 = SDL_SCANCODE_7, /* 7 */
		D8 = SDL_SCANCODE_8, /* 8 */
		D9 = SDL_SCANCODE_9, /* 9 */

		Semicolon = SDL_SCANCODE_SEMICOLON, /* ; */
		Equal = SDL_SCANCODE_EQUALS, /* = */

		A = SDL_SCANCODE_A,
		B = SDL_SCANCODE_B,
		C = SDL_SCANCODE_C,
		D = SDL_SCANCODE_D,
		E = SDL_SCANCODE_E,
		F = SDL_SCANCODE_F,
		G = SDL_SCANCODE_G,
		H = SDL_SCANCODE_H,
		I = SDL_SCANCODE_I,
		J = SDL_SCANCODE_J,
		K = SDL_SCANCODE_K,
		L = SDL_SCANCODE_L,
		M = SDL_SCANCODE_M,
		N = SDL_SCANCODE_N,
		O = SDL_SCANCODE_O,
		P = SDL_SCANCODE_P,
		Q = SDL_SCANCODE_Q,
		R = SDL_SCANCODE_R,
		S = SDL_SCANCODE_S,
		T = SDL_SCANCODE_T,
		U = SDL_SCANCODE_U,
		V = SDL_SCANCODE_V,
		W = SDL_SCANCODE_W,
		X = SDL_SCANCODE_X,
		Y = SDL_SCANCODE_Y,
		Z = SDL_SCANCODE_Z,

		LeftBracket = SDL_SCANCODE_LEFTBRACKET,  /* [ */
		Backslash = SDL_SCANCODE_BACKSLASH,  /* \ */
		RightBracket = SDL_SCANCODE_RIGHTBRACKET,  /* ] */
		GraveAccent = SDL_SCANCODE_GRAVE,  /* ` */

		World1 = SDL_SCANCODE_INTERNATIONAL1, /* non-US #1 */
		World2 = SDL_SCANCODE_INTERNATIONAL2, /* non-US #2 */

		/* Function keys */
		Escape = SDL_SCANCODE_ESCAPE,
		Enter = SDL_SCANCODE_RETURN,
		Tab = SDL_SCANCODE_TAB,
		Backspace = SDL_SCANCODE_BACKSPACE,
		Insert = SDL_SCANCODE_INSERT,
		Delete = SDL_SCANCODE_DELETE,
		Right = SDL_SCANCODE_RIGHT,
		Left = SDL_SCANCODE_LEFT,
		Down = SDL_SCANCODE_DOWN,
		Up = SDL_SCANCODE_UP,
		PageUp = SDL_SCANCODE_PAGEUP,
		PageDown = SDL_SCANCODE_PAGEDOWN,
		Home = SDL_SCANCODE_HOME,
		End = SDL_SCANCODE_END,
		CapsLock = SDL_SCANCODE_CAPSLOCK,
		ScrollLock = SDL_SCANCODE_SCROLLLOCK,
		NumLock = SDL_SCANCODE_NUMLOCKCLEAR,
		PrintScreen = SDL_SCANCODE_PRINTSCREEN,
		Pause = SDL_SCANCODE_PAUSE,
		F1 = SDL_SCANCODE_F1,
		F2 = SDL_SCANCODE_F2,
		F3 = SDL_SCANCODE_F3,
		F4 = SDL_SCANCODE_F4,
		F5 = SDL_SCANCODE_F5,
		F6 = SDL_SCANCODE_F6,
		F7 = SDL_SCANCODE_F7,
		F8 = SDL_SCANCODE_F8,
		F9 = SDL_SCANCODE_F9,
		F10 = SDL_SCANCODE_F10,
		F11 = SDL_SCANCODE_F11,
		F12 = SDL_SCANCODE_F12,
		F13 = SDL_SCANCODE_F13,
		F14 = SDL_SCANCODE_F14,
		F15 = SDL_SCANCODE_F15,
		F16 = SDL_SCANCODE_F16,
		F17 = SDL_SCANCODE_F17,
		F18 = SDL_SCANCODE_F18,
		F19 = SDL_SCANCODE_F19,
		F20 = SDL_SCANCODE_F20,
		F21 = SDL_SCANCODE_F21,
		F22 = SDL_SCANCODE_F22,
		F23 = SDL_SCANCODE_F23,
		F24 = SDL_SCANCODE_F24,

		/* Keypad */
		KP0 = SDL_SCANCODE_KP_0,
		KP1 = SDL_SCANCODE_KP_1,
		KP2 = SDL_SCANCODE_KP_2,
		KP3 = SDL_SCANCODE_KP_3,
		KP4 = SDL_SCANCODE_KP_4,
		KP5 = SDL_SCANCODE_KP_5,
		KP6 = SDL_SCANCODE_KP_6,
		KP7 = SDL_SCANCODE_KP_7,
		KP8 = SDL_SCANCODE_KP_8,
		KP9 = SDL_SCANCODE_KP_9,
		KPDecimal = SDL_SCANCODE_KP_PERIOD,
		KPDivide = SDL_SCANCODE_KP_DIVIDE,
		KPMultiply = SDL_SCANCODE_KP_MULTIPLY,
		KPSubtract = SDL_SCANCODE_KP_MINUS,
		KPAdd = SDL_SCANCODE_KP_PLUS,
		KPEnter = SDL_SCANCODE_KP_ENTER,
		KPEqual = SDL_SCANCODE_KP_EQUALS,

		LeftShift = SDL_SCANCODE_LSHIFT,
		LeftControl = SDL_SCANCODE_LCTRL,
		LeftAlt = SDL_SCANCODE_LALT,
		LeftSuper = SDL_SCANCODE_LGUI,
		RightShift = SDL_SCANCODE_RSHIFT,
		RightControl = SDL_SCANCODE_RCTRL,
		RightAlt = SDL_SCANCODE_RALT,
		RightSuper = SDL_SCANCODE_RGUI,
		Menu = SDL_SCANCODE_MENU
	} Key;

	enum class KeyState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};

	enum class CursorMode
	{
		Normal = 0,
		Hidden = 1,
		Locked = 2
	};

	typedef enum class MouseButton : uint8_t
	{
		Button0 = SDL_BUTTON_LEFT - 1,    // SDL buttons are 1-indexed, we make them 0-indexed
		Button1 = SDL_BUTTON_RIGHT - 1,
		Button2 = SDL_BUTTON_MIDDLE - 1,
		Button3 = SDL_BUTTON_X1 - 1,
		Button4 = SDL_BUTTON_X2 - 1,
		Button5 = 5, // Additional button
		Left = Button0,
		Right = Button1,
		Middle = Button2
	} Button;


	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<uint32_t>(keyCode);
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, MouseButton button)
	{
		os << static_cast<int32_t>(button);
		return os;
	}
}

// SDL3 key mappings
#define ZN_KEY_SPACE           ::Zenith::Key::Space
#define ZN_KEY_APOSTROPHE      ::Zenith::Key::Apostrophe    /* ' */
#define ZN_KEY_COMMA           ::Zenith::Key::Comma         /* , */
#define ZN_KEY_MINUS           ::Zenith::Key::Minus         /* - */
#define ZN_KEY_PERIOD          ::Zenith::Key::Period        /* . */
#define ZN_KEY_SLASH           ::Zenith::Key::Slash         /* / */
#define ZN_KEY_0               ::Zenith::Key::D0
#define ZN_KEY_1               ::Zenith::Key::D1
#define ZN_KEY_2               ::Zenith::Key::D2
#define ZN_KEY_3               ::Zenith::Key::D3
#define ZN_KEY_4               ::Zenith::Key::D4
#define ZN_KEY_5               ::Zenith::Key::D5
#define ZN_KEY_6               ::Zenith::Key::D6
#define ZN_KEY_7               ::Zenith::Key::D7
#define ZN_KEY_8               ::Zenith::Key::D8
#define ZN_KEY_9               ::Zenith::Key::D9
#define ZN_KEY_SEMICOLON       ::Zenith::Key::Semicolon     /* ; */
#define ZN_KEY_EQUAL           ::Zenith::Key::Equal         /* = */
#define ZN_KEY_A               ::Zenith::Key::A
#define ZN_KEY_B               ::Zenith::Key::B
#define ZN_KEY_C               ::Zenith::Key::C
#define ZN_KEY_D               ::Zenith::Key::D
#define ZN_KEY_E               ::Zenith::Key::E
#define ZN_KEY_F               ::Zenith::Key::F
#define ZN_KEY_G               ::Zenith::Key::G
#define ZN_KEY_H               ::Zenith::Key::H
#define ZN_KEY_I               ::Zenith::Key::I
#define ZN_KEY_J               ::Zenith::Key::J
#define ZN_KEY_K               ::Zenith::Key::K
#define ZN_KEY_L               ::Zenith::Key::L
#define ZN_KEY_M               ::Zenith::Key::M
#define ZN_KEY_N               ::Zenith::Key::N
#define ZN_KEY_O               ::Zenith::Key::O
#define ZN_KEY_P               ::Zenith::Key::P
#define ZN_KEY_Q               ::Zenith::Key::Q
#define ZN_KEY_R               ::Zenith::Key::R
#define ZN_KEY_S               ::Zenith::Key::S
#define ZN_KEY_T               ::Zenith::Key::T
#define ZN_KEY_U               ::Zenith::Key::U
#define ZN_KEY_V               ::Zenith::Key::V
#define ZN_KEY_W               ::Zenith::Key::W
#define ZN_KEY_X               ::Zenith::Key::X
#define ZN_KEY_Y               ::Zenith::Key::Y
#define ZN_KEY_Z               ::Zenith::Key::Z
#define ZN_KEY_LEFT_BRACKET    ::Zenith::Key::LeftBracket   /* [ */
#define ZN_KEY_BACKSLASH       ::Zenith::Key::Backslash     /* \ */
#define ZN_KEY_RIGHT_BRACKET   ::Zenith::Key::RightBracket  /* ] */
#define ZN_KEY_GRAVE_ACCENT    ::Zenith::Key::GraveAccent   /* ` */
#define ZN_KEY_WORLD_1         ::Zenith::Key::World1        /* non-US #1 */
#define ZN_KEY_WORLD_2         ::Zenith::Key::World2        /* non-US #2 */

/* Function keys */
#define ZN_KEY_ESCAPE          ::Zenith::Key::Escape
#define ZN_KEY_ENTER           ::Zenith::Key::Enter
#define ZN_KEY_TAB             ::Zenith::Key::Tab
#define ZN_KEY_BACKSPACE       ::Zenith::Key::Backspace
#define ZN_KEY_INSERT          ::Zenith::Key::Insert
#define ZN_KEY_DELETE          ::Zenith::Key::Delete
#define ZN_KEY_RIGHT           ::Zenith::Key::Right
#define ZN_KEY_LEFT            ::Zenith::Key::Left
#define ZN_KEY_DOWN            ::Zenith::Key::Down
#define ZN_KEY_UP              ::Zenith::Key::Up
#define ZN_KEY_PAGE_UP         ::Zenith::Key::PageUp
#define ZN_KEY_PAGE_DOWN       ::Zenith::Key::PageDown
#define ZN_KEY_HOME            ::Zenith::Key::Home
#define ZN_KEY_END             ::Zenith::Key::End
#define ZN_KEY_CAPS_LOCK       ::Zenith::Key::CapsLock
#define ZN_KEY_SCROLL_LOCK     ::Zenith::Key::ScrollLock
#define ZN_KEY_NUM_LOCK        ::Zenith::Key::NumLock
#define ZN_KEY_PRINT_SCREEN    ::Zenith::Key::PrintScreen
#define ZN_KEY_PAUSE           ::Zenith::Key::Pause
#define ZN_KEY_F1              ::Zenith::Key::F1
#define ZN_KEY_F2              ::Zenith::Key::F2
#define ZN_KEY_F3              ::Zenith::Key::F3
#define ZN_KEY_F4              ::Zenith::Key::F4
#define ZN_KEY_F5              ::Zenith::Key::F5
#define ZN_KEY_F6              ::Zenith::Key::F6
#define ZN_KEY_F7              ::Zenith::Key::F7
#define ZN_KEY_F8              ::Zenith::Key::F8
#define ZN_KEY_F9              ::Zenith::Key::F9
#define ZN_KEY_F10             ::Zenith::Key::F10
#define ZN_KEY_F11             ::Zenith::Key::F11
#define ZN_KEY_F12             ::Zenith::Key::F12
#define ZN_KEY_F13             ::Zenith::Key::F13
#define ZN_KEY_F14             ::Zenith::Key::F14
#define ZN_KEY_F15             ::Zenith::Key::F15
#define ZN_KEY_F16             ::Zenith::Key::F16
#define ZN_KEY_F17             ::Zenith::Key::F17
#define ZN_KEY_F18             ::Zenith::Key::F18
#define ZN_KEY_F19             ::Zenith::Key::F19
#define ZN_KEY_F20             ::Zenith::Key::F20
#define ZN_KEY_F21             ::Zenith::Key::F21
#define ZN_KEY_F22             ::Zenith::Key::F22
#define ZN_KEY_F23             ::Zenith::Key::F23
#define ZN_KEY_F24             ::Zenith::Key::F24
#define ZN_KEY_F25             ::Zenith::Key::F25

/* Keypad */
#define ZN_KEY_KP_0            ::Zenith::Key::KP0
#define ZN_KEY_KP_1            ::Zenith::Key::KP1
#define ZN_KEY_KP_2            ::Zenith::Key::KP2
#define ZN_KEY_KP_3            ::Zenith::Key::KP3
#define ZN_KEY_KP_4            ::Zenith::Key::KP4
#define ZN_KEY_KP_5            ::Zenith::Key::KP5
#define ZN_KEY_KP_6            ::Zenith::Key::KP6
#define ZN_KEY_KP_7            ::Zenith::Key::KP7
#define ZN_KEY_KP_8            ::Zenith::Key::KP8
#define ZN_KEY_KP_9            ::Zenith::Key::KP9
#define ZN_KEY_KP_DECIMAL      ::Zenith::Key::KPDecimal
#define ZN_KEY_KP_DIVIDE       ::Zenith::Key::KPDivide
#define ZN_KEY_KP_MULTIPLY     ::Zenith::Key::KPMultiply
#define ZN_KEY_KP_SUBTRACT     ::Zenith::Key::KPSubtract
#define ZN_KEY_KP_ADD          ::Zenith::Key::KPAdd
#define ZN_KEY_KP_ENTER        ::Zenith::Key::KPEnter
#define ZN_KEY_KP_EQUAL        ::Zenith::Key::KPEqual

#define ZN_KEY_LEFT_SHIFT      ::Zenith::Key::LeftShift
#define ZN_KEY_LEFT_CONTROL    ::Zenith::Key::LeftControl
#define ZN_KEY_LEFT_ALT        ::Zenith::Key::LeftAlt
#define ZN_KEY_LEFT_SUPER      ::Zenith::Key::LeftSuper
#define ZN_KEY_RIGHT_SHIFT     ::Zenith::Key::RightShift
#define ZN_KEY_RIGHT_CONTROL   ::Zenith::Key::RightControl
#define ZN_KEY_RIGHT_ALT       ::Zenith::Key::RightAlt
#define ZN_KEY_RIGHT_SUPER     ::Zenith::Key::RightSuper
#define ZN_KEY_MENU            ::Zenith::Key::Menu

// Mouse
#define ZN_MOUSE_BUTTON_LEFT    ::Zenith::Button::Left
#define ZN_MOUSE_BUTTON_RIGHT   ::Zenith::Button::Right
#define ZN_MOUSE_BUTTON_MIDDLE  ::Zenith::Button::Middle