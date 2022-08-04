// PlatformKeyboard.h

#pragma once

#include "IKeyboard.h"
#include "PlatformInput.h"

#include <map>

class PlatformKeyboard : public IKeyboard
{
    GLFWkeyfun _onKeyEvent;
    // I wanted to carry over the array implementation in DirectInput, 
    // but I'm not sure the same would work on other platforms
    std::map<KEYBOARD_KEYS, bool> _keyStates;
    std::map<KEYBOARD_KEYS, bool> _keyStatesLast;

protected:
    void _onKeyEventHandler(GLFWwindow* window, int key, int scancode, int action, int mods);

public:
    PlatformKeyboard(Window *window);

    void Release(void) {}

    bool KeyDown(int key);
    bool KeyUp(int key);
    bool KeyPressed(int key);
    bool KeyReleased(int key);
};