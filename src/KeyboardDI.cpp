#ifdef _WIN32

#include "KeyboardDI.h"

namespace {
const KeyMapping kDIKeys[] = {
	{ Key::Escape, DIK_ESCAPE },
	{ Key::Num0, DIK_0 },
	{ Key::Num1, DIK_1 },
	{ Key::Num2, DIK_2 },
	{ Key::Num3, DIK_3 },
	{ Key::Num4, DIK_4 },
	{ Key::Num5, DIK_5 },
	{ Key::Num6, DIK_6 },
	{ Key::Num7, DIK_7 },
	{ Key::Num8, DIK_8 },
	{ Key::Num9, DIK_9 },
	{ Key::A, DIK_A },
	{ Key::B, DIK_B },
	{ Key::C, DIK_C },
	{ Key::D, DIK_D },
	{ Key::E, DIK_E },
	{ Key::F, DIK_F },
	{ Key::G, DIK_G },
	{ Key::H, DIK_H },
	{ Key::I, DIK_I },
	{ Key::J, DIK_J },
	{ Key::K, DIK_K },
	{ Key::L, DIK_L },
	{ Key::M, DIK_M },
	{ Key::N, DIK_N },
	{ Key::O, DIK_O },
	{ Key::P, DIK_P },
	{ Key::Q, DIK_Q },
	{ Key::R, DIK_R },
	{ Key::S, DIK_S },
	{ Key::T, DIK_T },
	{ Key::U, DIK_U },
	{ Key::V, DIK_V },
	{ Key::W, DIK_W },
	{ Key::X, DIK_X },
	{ Key::Y, DIK_Y },
	{ Key::Z, DIK_Z },
	{ Key::F1, DIK_F1 },
	{ Key::F2, DIK_F2 },
	{ Key::F3, DIK_F3 },
	{ Key::F4, DIK_F4 },
	{ Key::F5, DIK_F5 },
	{ Key::F6, DIK_F6 },
	{ Key::F7, DIK_F7 },
	{ Key::F8, DIK_F8 },
	{ Key::F9, DIK_F9 },
	{ Key::F10, DIK_F10 },
	{ Key::F11, DIK_F11 },
	{ Key::F12, DIK_F12 },
	{ Key::F13, DIK_F13 },
	{ Key::F14, DIK_F14 },
	{ Key::F15, DIK_F15 },
	{ Key::Space, DIK_SPACE },
	{ Key::Enter, DIK_RETURN },
	{ Key::Tab, DIK_TAB },
	{ Key::Backspace, DIK_BACK },
	{ Key::Insert, DIK_INSERT },
	{ Key::Delete, DIK_DELETE },
	{ Key::Home, DIK_HOME },
	{ Key::End, DIK_END },
	{ Key::PageUp, DIK_PRIOR },
	{ Key::PageDown, DIK_NEXT },
	{ Key::Up, DIK_UP },
	{ Key::Down, DIK_DOWN },
	{ Key::Left, DIK_LEFT },
	{ Key::Right, DIK_RIGHT },
	{ Key::Minus, DIK_MINUS },
	{ Key::Equals, DIK_EQUALS },
	{ Key::LeftBracket, DIK_LBRACKET },
	{ Key::RightBracket, DIK_RBRACKET },
	{ Key::Backslash, DIK_BACKSLASH },
	{ Key::Semicolon, DIK_SEMICOLON },
	{ Key::Apostrophe, DIK_APOSTROPHE },
	{ Key::Grave, DIK_GRAVE },
	{ Key::Comma, DIK_COMMA },
	{ Key::Period, DIK_PERIOD },
	{ Key::Slash, DIK_SLASH },
	{ Key::LeftShift, DIK_LSHIFT },
	{ Key::RightShift, DIK_RSHIFT },
	{ Key::LeftControl, DIK_LCONTROL },
	{ Key::RightControl, DIK_RCONTROL },
	{ Key::LeftAlt, DIK_LMENU },
	{ Key::RightAlt, DIK_RMENU },
	{ Key::LeftSuper, DIK_LWIN },
	{ Key::RightSuper, DIK_RWIN },
	{ Key::Menu, DIK_APPS },
	{ Key::CapsLock, DIK_CAPITAL },
	{ Key::NumLock, DIK_NUMLOCK },
	{ Key::ScrollLock, DIK_SCROLL },
	{ Key::PrintScreen, DIK_SYSRQ },
	{ Key::Pause, DIK_PAUSE },
	{ Key::Keypad0, DIK_NUMPAD0 },
	{ Key::Keypad1, DIK_NUMPAD1 },
	{ Key::Keypad2, DIK_NUMPAD2 },
	{ Key::Keypad3, DIK_NUMPAD3 },
	{ Key::Keypad4, DIK_NUMPAD4 },
	{ Key::Keypad5, DIK_NUMPAD5 },
	{ Key::Keypad6, DIK_NUMPAD6 },
	{ Key::Keypad7, DIK_NUMPAD7 },
	{ Key::Keypad8, DIK_NUMPAD8 },
	{ Key::Keypad9, DIK_NUMPAD9 },
	{ Key::KeypadDecimal, DIK_DECIMAL },
	{ Key::KeypadDivide, DIK_DIVIDE },
	{ Key::KeypadMultiply, DIK_MULTIPLY },
	{ Key::KeypadSubtract, DIK_SUBTRACT },
	{ Key::KeypadAdd, DIK_ADD },
	{ Key::KeypadEnter, DIK_NUMPADENTER },
	{ Key::KeypadEquals, DIK_NUMPADEQUALS },
};
}

KeyboardDI::KeyboardDI(void)
{
	memset(m_cKeyBuffer, 0, _countof(m_cKeyBuffer));
}

bool KeyboardDI::acquire(LPDIRECTINPUT8 pDI, HWND hWnd)
{
	if (!FAILED(pDI->CreateDevice(GUID_SysKeyboard, &m_lpDevice, NULL))) {

		m_lpDevice->SetDataFormat(&c_dfDIKeyboard);
		m_lpDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

		return true;
	}
	return false;
}

void KeyboardDI::update(void)
{
	IDIDevice::update();

	if (m_lpDevice) {

		if (m_lpDevice->GetDeviceState(sizeof(m_cKeyBuffer), (LPVOID)&m_cKeyBuffer) == DIERR_INPUTLOST)
			m_bDeviceLost = true;

		_keys.beginFrame();
		for (const KeyMapping& mapping : kDIKeys) {
			_keys.set((int)mapping.key, (m_cKeyBuffer[mapping.native] & 0x80) != 0);
		}
	}
}

#endif
