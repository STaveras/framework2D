// Key.h
// Keyboard keys, independent of platform. Each keyboard backend maps its
// native codes (GLFW keys, DirectInput scan codes, USB HID usages) to these
// with a KeyMapping table, so game code and bindings never see native codes.

#pragma once

#include <cstddef>
#include <cstdint>

enum class Key : uint16_t
{
	Unknown,

	Escape,
	Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
	A, B, C, D, E, F, G, H, I, J, K, L, M,
	N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,

	Space, Enter, Tab, Backspace,
	Insert, Delete, Home, End, PageUp, PageDown,
	Up, Down, Left, Right,

	Minus, Equals, LeftBracket, RightBracket, Backslash,
	Semicolon, Apostrophe, Grave, Comma, Period, Slash,

	LeftShift, RightShift, LeftControl, RightControl,
	LeftAlt, RightAlt, LeftSuper, RightSuper, Menu,
	CapsLock, NumLock, ScrollLock, PrintScreen, Pause,

	Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
	Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
	KeypadDecimal, KeypadDivide, KeypadMultiply, KeypadSubtract,
	KeypadAdd, KeypadEnter, KeypadEquals,

	Count
};

// A keyboard backend's native code for one Key.
struct KeyMapping
{
	Key key;
	int native;
};

// The Key a native code maps to in table, or Key::Unknown.
template <size_t N>
Key findKey(const KeyMapping (&table)[N], int native)
{
	for (const KeyMapping& mapping : table) {
		if (mapping.native == native) {
			return mapping.key;
		}
	}
	return Key::Unknown;
}
