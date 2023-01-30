#pragma once

#include "Types.h"

namespace framework {

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#define KEYBOARD Keyboard* keyboard = Engine2D::getInput()->getKeyboard()

	typedef class IKeyboard
	{
	public:
		typedef int32_t KEY;

		struct KEYS {
			KEY KBK_ESCAPE,
				KBK_1,
				KBK_2,
				KBK_3,
				KBK_4,
				KBK_5,
				KBK_6,
				KBK_7,
				KBK_8,
				KBK_9,
				KBK_0,
				KBK_MINUS,
				KBK_EQUALS,
				KBK_BACK,
				KBK_TAB,
				KBK_Q,
				KBK_W,
				KBK_E,
				KBK_R,
				KBK_T,
				KBK_Y,
				KBK_U,
				KBK_I,
				KBK_O,
				KBK_P,
				KBK_LBRACKET,
				KBK_RBRACKET,
				KBK_RETURN,
				KBK_LCONTROL,
				KBK_A,
				KBK_S,
				KBK_D,
				KBK_F,
				KBK_G,
				KBK_H,
				KBK_J,
				KBK_K,
				KBK_L,
				KBK_SEMICOLON,
				KBK_APOSTROPHE,
				KBK_GRAVE,
				KBK_LSHIFT,
				KBK_BACKSLASH,
				KBK_Z,
				KBK_X,
				KBK_C,
				KBK_V,
				KBK_B,
				KBK_N,
				KBK_M,
				KBK_COMMA,
				KBK_PERIOD,
				KBK_SLASH,
				KBK_RSHIFT,
				KBK_MULTIPLY,
				KBK_LMENU,
				KBK_SPACE,
				KBK_CAPITAL,
				KBK_F1,
				KBK_F2,
				KBK_F3,
				KBK_F4,
				KBK_F5,
				KBK_F6,
				KBK_F7,
				KBK_F8,
				KBK_F9,
				KBK_F10,
				KBK_NUMLOCK,
				KBK_SCROLL,
				KBK_NUMPAD7,
				KBK_NUMPAD8,
				KBK_NUMPAD9,
				KBK_SUBTRACT,
				KBK_NUMPAD4,
				KBK_NUMPAD5,
				KBK_NUMPAD6,
				KBK_ADD,
				KBK_NUMPAD1,
				KBK_NUMPAD2,
				KBK_NUMPAD3,
				KBK_NUMPAD0,
				KBK_DECIMAL,
				KBK_OEM_102,
				KBK_F11,
				KBK_F12,
				KBK_F13,
				KBK_F14,
				KBK_F15,
				KBK_KANA,
				KBK_ABNT_C1,
				KBK_CONVERT,
				KBK_NOCONVERT,
				KBK_YEN,
				KBK_ABNT_C2,
				KBK_NUMPADEQUALS,
				KBK_PREVTRACK,
				KBK_AT,
				KBK_COLON,
				KBK_UNDERLINE,
				KBK_KANJI,
				KBK_STOP,
				KBK_AX,
				KBK_UNLABELED,
				KBK_NEXTTRACK,
				KBK_NUMPADENTER,
				KBK_RCONTROL,
				KBK_MUTE,
				KBK_CALCULATOR,
				KBK_PLAYPAUSE,
				KBK_MEDIASTOP,
				KBK_VOLUMEDOWN,
				KBK_VOLUMEUP,
				KBK_WEBHOME,
				KBK_NUMPADCOMMA,
				KBK_DIVIDE,
				KBK_SYSRQ,
				KBK_RMENU,
				KBK_PAUSE,
				KBK_HOME,
				KBK_UP,
				KBK_PGUP,
				KBK_LEFT,
				KBK_RIGHT,
				KBK_END,
				KBK_DOWN,
				KBK_PGDN,
				KBK_INSERT,
				KBK_DELETE,
				KBK_LWIN,
				KBK_RWIN,
				KBK_APPS,
				KBK_POWER,
				KBK_SLEEP,
				KBK_WAKE,
				KBK_WEBSEARCH,
				KBK_WEBFAVORITES,
				KBK_WEBREFRESH,
				KBK_WEBSTOP,
				KBK_WEBFORWARD,
				KBK_WEBBACK,
				KBK_MYCOMPUTER,
				KBK_MAIL,
				KBK_MEDIASELECT;
		}keys;

		virtual const KEYS& getKeys(void) const { return (*this).keys; }

		virtual bool keyDown(KEY nKey) = 0;
		virtual bool keyUp(KEY nKey) = 0;
		virtual bool keyPressed(KEY nKey) = 0;
		virtual bool keyReleased(KEY nKey) = 0;

		//virtual void Capture(void) = 0;
		//virtual void release(void) = 0;

		virtual ~IKeyboard(void) = default;

	}Keyboard;

	typedef Keyboard KeyboardInterface;

#endif

}

using framework::Keyboard;