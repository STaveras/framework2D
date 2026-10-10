#include "PlatformKeyboard.h"

#include "Window.h"

namespace {
const KeyMapping kGLFWKeys[] = {
	{ Key::Escape, GLFW_KEY_ESCAPE },
	{ Key::Num0, GLFW_KEY_0 },
	{ Key::Num1, GLFW_KEY_1 },
	{ Key::Num2, GLFW_KEY_2 },
	{ Key::Num3, GLFW_KEY_3 },
	{ Key::Num4, GLFW_KEY_4 },
	{ Key::Num5, GLFW_KEY_5 },
	{ Key::Num6, GLFW_KEY_6 },
	{ Key::Num7, GLFW_KEY_7 },
	{ Key::Num8, GLFW_KEY_8 },
	{ Key::Num9, GLFW_KEY_9 },
	{ Key::A, GLFW_KEY_A },
	{ Key::B, GLFW_KEY_B },
	{ Key::C, GLFW_KEY_C },
	{ Key::D, GLFW_KEY_D },
	{ Key::E, GLFW_KEY_E },
	{ Key::F, GLFW_KEY_F },
	{ Key::G, GLFW_KEY_G },
	{ Key::H, GLFW_KEY_H },
	{ Key::I, GLFW_KEY_I },
	{ Key::J, GLFW_KEY_J },
	{ Key::K, GLFW_KEY_K },
	{ Key::L, GLFW_KEY_L },
	{ Key::M, GLFW_KEY_M },
	{ Key::N, GLFW_KEY_N },
	{ Key::O, GLFW_KEY_O },
	{ Key::P, GLFW_KEY_P },
	{ Key::Q, GLFW_KEY_Q },
	{ Key::R, GLFW_KEY_R },
	{ Key::S, GLFW_KEY_S },
	{ Key::T, GLFW_KEY_T },
	{ Key::U, GLFW_KEY_U },
	{ Key::V, GLFW_KEY_V },
	{ Key::W, GLFW_KEY_W },
	{ Key::X, GLFW_KEY_X },
	{ Key::Y, GLFW_KEY_Y },
	{ Key::Z, GLFW_KEY_Z },
	{ Key::F1, GLFW_KEY_F1 },
	{ Key::F2, GLFW_KEY_F2 },
	{ Key::F3, GLFW_KEY_F3 },
	{ Key::F4, GLFW_KEY_F4 },
	{ Key::F5, GLFW_KEY_F5 },
	{ Key::F6, GLFW_KEY_F6 },
	{ Key::F7, GLFW_KEY_F7 },
	{ Key::F8, GLFW_KEY_F8 },
	{ Key::F9, GLFW_KEY_F9 },
	{ Key::F10, GLFW_KEY_F10 },
	{ Key::F11, GLFW_KEY_F11 },
	{ Key::F12, GLFW_KEY_F12 },
	{ Key::F13, GLFW_KEY_F13 },
	{ Key::F14, GLFW_KEY_F14 },
	{ Key::F15, GLFW_KEY_F15 },
	{ Key::Space, GLFW_KEY_SPACE },
	{ Key::Enter, GLFW_KEY_ENTER },
	{ Key::Tab, GLFW_KEY_TAB },
	{ Key::Backspace, GLFW_KEY_BACKSPACE },
	{ Key::Insert, GLFW_KEY_INSERT },
	{ Key::Delete, GLFW_KEY_DELETE },
	{ Key::Home, GLFW_KEY_HOME },
	{ Key::End, GLFW_KEY_END },
	{ Key::PageUp, GLFW_KEY_PAGE_UP },
	{ Key::PageDown, GLFW_KEY_PAGE_DOWN },
	{ Key::Up, GLFW_KEY_UP },
	{ Key::Down, GLFW_KEY_DOWN },
	{ Key::Left, GLFW_KEY_LEFT },
	{ Key::Right, GLFW_KEY_RIGHT },
	{ Key::Minus, GLFW_KEY_MINUS },
	{ Key::Equals, GLFW_KEY_EQUAL },
	{ Key::LeftBracket, GLFW_KEY_LEFT_BRACKET },
	{ Key::RightBracket, GLFW_KEY_RIGHT_BRACKET },
	{ Key::Backslash, GLFW_KEY_BACKSLASH },
	{ Key::Semicolon, GLFW_KEY_SEMICOLON },
	{ Key::Apostrophe, GLFW_KEY_APOSTROPHE },
	{ Key::Grave, GLFW_KEY_GRAVE_ACCENT },
	{ Key::Comma, GLFW_KEY_COMMA },
	{ Key::Period, GLFW_KEY_PERIOD },
	{ Key::Slash, GLFW_KEY_SLASH },
	{ Key::LeftShift, GLFW_KEY_LEFT_SHIFT },
	{ Key::RightShift, GLFW_KEY_RIGHT_SHIFT },
	{ Key::LeftControl, GLFW_KEY_LEFT_CONTROL },
	{ Key::RightControl, GLFW_KEY_RIGHT_CONTROL },
	{ Key::LeftAlt, GLFW_KEY_LEFT_ALT },
	{ Key::RightAlt, GLFW_KEY_RIGHT_ALT },
	{ Key::LeftSuper, GLFW_KEY_LEFT_SUPER },
	{ Key::RightSuper, GLFW_KEY_RIGHT_SUPER },
	{ Key::Menu, GLFW_KEY_MENU },
	{ Key::CapsLock, GLFW_KEY_CAPS_LOCK },
	{ Key::NumLock, GLFW_KEY_NUM_LOCK },
	{ Key::ScrollLock, GLFW_KEY_SCROLL_LOCK },
	{ Key::PrintScreen, GLFW_KEY_PRINT_SCREEN },
	{ Key::Pause, GLFW_KEY_PAUSE },
	{ Key::Keypad0, GLFW_KEY_KP_0 },
	{ Key::Keypad1, GLFW_KEY_KP_1 },
	{ Key::Keypad2, GLFW_KEY_KP_2 },
	{ Key::Keypad3, GLFW_KEY_KP_3 },
	{ Key::Keypad4, GLFW_KEY_KP_4 },
	{ Key::Keypad5, GLFW_KEY_KP_5 },
	{ Key::Keypad6, GLFW_KEY_KP_6 },
	{ Key::Keypad7, GLFW_KEY_KP_7 },
	{ Key::Keypad8, GLFW_KEY_KP_8 },
	{ Key::Keypad9, GLFW_KEY_KP_9 },
	{ Key::KeypadDecimal, GLFW_KEY_KP_DECIMAL },
	{ Key::KeypadDivide, GLFW_KEY_KP_DIVIDE },
	{ Key::KeypadMultiply, GLFW_KEY_KP_MULTIPLY },
	{ Key::KeypadSubtract, GLFW_KEY_KP_SUBTRACT },
	{ Key::KeypadAdd, GLFW_KEY_KP_ADD },
	{ Key::KeypadEnter, GLFW_KEY_KP_ENTER },
	{ Key::KeypadEquals, GLFW_KEY_KP_EQUAL },
};
}

PlatformKeyboard::PlatformKeyboard(Window* window) :
	_owner(window),
	_window(window ? window->getUnderlyingWindow() : nullptr)
{
	if (_owner && _window) {
		_owner->setKeyEventHandler([this](int key, int scancode, int action, int mods) {
			(void)scancode;
			(void)mods;
			_onKeyEvent(key, action);
		});
	}
}

PlatformKeyboard::~PlatformKeyboard(void)
{
	if (_owner && _window) {
		_owner->setKeyEventHandler(nullptr);
	}
}

void PlatformKeyboard::_onKeyEvent(int key, int action)
{
	// Latch presses until the next update() so a press and release that both
	// land inside one glfwPollEvents() still reads as down for a frame
	const Key mapped = findKey(kGLFWKeys, key);
	if (action == GLFW_PRESS && mapped != Key::Unknown) {
		_keys.latchPress((int)mapped);
	}
}

void PlatformKeyboard::update(void)
{
	if (!_window) {
		return;
	}

	_keys.beginFrame();
	for (const KeyMapping& mapping : kGLFWKeys) {
		const int state = glfwGetKey(_window, mapping.native);
		_keys.set((int)mapping.key, state == GLFW_PRESS || state == GLFW_REPEAT);
	}
}
