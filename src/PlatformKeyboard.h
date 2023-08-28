// PlatformKeyboard.h

#pragma once

#include "Keyboard.h"
#include "PlatformInput.h"

#include <map>

// Keyboard input based on GLFW
class PlatformKeyboard : public Keyboard
{
    GLFWkeyfun _onKeyEvent;
    // I wanted to carry over the array implementation in DirectInput, 
    // but I'm not sure the same would work on other platforms
    std::map<KEY, bool> _keyStates;
    std::map<KEY, bool> _keyStatesLast;

public:
   // TODO: Expose key definitions

protected:
    void _onKeyEventHandler(GLFWwindow* window, int key, int scancode, int action, int mods);

public:
    PlatformKeyboard(Window *window);

    void release(void) {}

    bool keyDown(KEY key);
    bool keyUp(KEY key);
    bool keyPressed(KEY key);
    bool keyReleased(KEY key);

    KEYS keys{
        GLFW_KEY_ESCAPE,
        GLFW_KEY_1,
        GLFW_KEY_2,
        GLFW_KEY_3,
        GLFW_KEY_4,
        GLFW_KEY_5,
        GLFW_KEY_6,
        GLFW_KEY_7,
        GLFW_KEY_8,
        GLFW_KEY_9,
        GLFW_KEY_0,
        GLFW_KEY_MINUS,
        GLFW_KEY_EQUAL,
        GLFW_KEY_BACKSPACE,
        GLFW_KEY_TAB,
        GLFW_KEY_Q,
        GLFW_KEY_W,
        GLFW_KEY_E,
        GLFW_KEY_R,
        GLFW_KEY_T,
        GLFW_KEY_Y,
        GLFW_KEY_U,
        GLFW_KEY_I,
        GLFW_KEY_O,
        GLFW_KEY_P,
        GLFW_KEY_LEFT_BRACKET,
        GLFW_KEY_RIGHT_BRACKET,
        GLFW_KEY_ENTER,
        GLFW_KEY_LEFT_CONTROL,
        GLFW_KEY_A,
        GLFW_KEY_S,
        GLFW_KEY_D,
        GLFW_KEY_F,
        GLFW_KEY_G,
        GLFW_KEY_H,
        GLFW_KEY_J,
        GLFW_KEY_K,
        GLFW_KEY_L,
        GLFW_KEY_SEMICOLON,
        GLFW_KEY_APOSTROPHE,
        GLFW_KEY_GRAVE_ACCENT,
        GLFW_KEY_LEFT_SHIFT,
        GLFW_KEY_BACKSLASH,
        GLFW_KEY_Z,
        GLFW_KEY_X,
        GLFW_KEY_C,
        GLFW_KEY_V,
        GLFW_KEY_B,
        GLFW_KEY_N,
        GLFW_KEY_M,
        GLFW_KEY_COMMA,
        GLFW_KEY_PERIOD,
        GLFW_KEY_SLASH,
        GLFW_KEY_RIGHT_SHIFT,
        GLFW_KEY_KP_MULTIPLY,
        GLFW_KEY_LEFT_ALT,
        GLFW_KEY_SPACE,
        GLFW_KEY_CAPS_LOCK,
        GLFW_KEY_F1,
        GLFW_KEY_F2,
        GLFW_KEY_F3,
        GLFW_KEY_F4,
        GLFW_KEY_F5,
        GLFW_KEY_F6,
        GLFW_KEY_F7,
        GLFW_KEY_F8,
        GLFW_KEY_F9,
        GLFW_KEY_F10,
        GLFW_KEY_NUM_LOCK,
        GLFW_KEY_SCROLL_LOCK,
        GLFW_KEY_KP_7,
        GLFW_KEY_KP_8,
        GLFW_KEY_KP_9,
        GLFW_KEY_KP_SUBTRACT,
        GLFW_KEY_KP_4,
        GLFW_KEY_KP_5,
        GLFW_KEY_KP_6,
        GLFW_KEY_KP_ADD,
        GLFW_KEY_KP_1,
        GLFW_KEY_KP_2,
        GLFW_KEY_KP_3,
        GLFW_KEY_KP_0,
        GLFW_KEY_KP_DECIMAL,
        GLFW_KEY_UNKNOWN, // DIK_OEM_102
        GLFW_KEY_F11,
        GLFW_KEY_F12,
        GLFW_KEY_F13,
        GLFW_KEY_F14,
        GLFW_KEY_F15,
        GLFW_KEY_UNKNOWN, // DIK_KANA
        GLFW_KEY_UNKNOWN, // DIK_ABNT_C1
        GLFW_KEY_UNKNOWN, // DIK_CONVERT
        GLFW_KEY_UNKNOWN, // DIK_NOCONVERT
        GLFW_KEY_UNKNOWN, // DIK_YEN
        GLFW_KEY_UNKNOWN, // DIK_ABNT_C2
        GLFW_KEY_KP_EQUAL,
        GLFW_KEY_UNKNOWN, // DIK_PREVTRACK
        GLFW_KEY_UNKNOWN, // DIK_AT
        GLFW_KEY_UNKNOWN, // DIK_COLON
        GLFW_KEY_UNKNOWN, // DIK_UNDERLINE
        GLFW_KEY_UNKNOWN, // DIK_KANJI
        GLFW_KEY_UNKNOWN, // DIK_STOP
        GLFW_KEY_UNKNOWN, // DIK_AX
        GLFW_KEY_UNKNOWN, // DIK_UNLABELED
        GLFW_KEY_UNKNOWN, // DIK_NEXTTRACK
        GLFW_KEY_KP_ENTER,
        GLFW_KEY_RIGHT_CONTROL,
        GLFW_KEY_UNKNOWN, // DIK_MUTE
        GLFW_KEY_UNKNOWN, // DIK_CALCULATOR
        GLFW_KEY_UNKNOWN, // DIK_PLAYPAUSE
        GLFW_KEY_UNKNOWN, // DIK_MEDIASTOP
        GLFW_KEY_UNKNOWN, // DIK_VOLUMEDOWN
        GLFW_KEY_UNKNOWN, // DIK_VOLUMEUP
        GLFW_KEY_UNKNOWN, // DIK_WEBHOME
        GLFW_KEY_KP_DECIMAL,
        GLFW_KEY_KP_DIVIDE,
        GLFW_KEY_PRINT_SCREEN,
        GLFW_KEY_RIGHT_ALT,
        GLFW_KEY_PAUSE,
        GLFW_KEY_HOME,
        GLFW_KEY_UP,
        GLFW_KEY_PAGE_UP,
        GLFW_KEY_LEFT,
        GLFW_KEY_RIGHT,
        GLFW_KEY_END,
        GLFW_KEY_DOWN,
        GLFW_KEY_PAGE_DOWN,
        GLFW_KEY_INSERT,
        GLFW_KEY_DELETE,
        GLFW_KEY_LEFT_SUPER,
        GLFW_KEY_RIGHT_SUPER,
        GLFW_KEY_MENU,
        GLFW_KEY_UNKNOWN, // DIK_POWER
        GLFW_KEY_UNKNOWN, // DIK_SLEEP
        GLFW_KEY_UNKNOWN, // DIK_WAKE
        GLFW_KEY_UNKNOWN, // DIK_WEBSEARCH
        GLFW_KEY_UNKNOWN, // DIK_WEBFAVORITES
        GLFW_KEY_UNKNOWN, // DIK_WEBREFRESH
        GLFW_KEY_UNKNOWN, // DIK_WEBSTOP
        GLFW_KEY_UNKNOWN, // DIK_WEBFORWARD
        GLFW_KEY_UNKNOWN, // DIK_WEBBACK
        GLFW_KEY_UNKNOWN, // DIK_MYCOMPUTER
        GLFW_KEY_UNKNOWN, // DIK_MAIL
        GLFW_KEY_UNKNOWN  // DIK_MEDIASELECT
    };

	const KEYS& getKeys(void) const { return (*this).keys; }
};