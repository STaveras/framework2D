
#include "PlatformKeyboard.h"

PlatformKeyboard::PlatformKeyboard(Window *window)
{
    // glfwSetKeyCallback(window->getUnderlyingWindow(), (GLFWkeyfun)(&PlatformKeyboard::_onKeyEventHandler));
    // _onKeyEvent

    glfwSetKeyCallback(window->getUnderlyingWindow(), _onKeyEvent);
}

void PlatformKeyboard::_onKeyEventHandler(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    _keyStatesLast[(KEYBOARD_KEYS)key] = _keyStates[(KEYBOARD_KEYS)key];
    _keyStates[(KEYBOARD_KEYS)key] = (action == GLFW_RELEASE) ? false : true;    
}

bool PlatformKeyboard::KeyDown(int key)
{
    return _keyStates[(KEYBOARD_KEYS)key];
}

bool PlatformKeyboard::KeyUp(int key)
{
    return !_keyStates[(KEYBOARD_KEYS)key];
}

bool PlatformKeyboard::KeyPressed(int key)
{
    return _keyStatesLast[(KEYBOARD_KEYS)key] != _keyStates[(KEYBOARD_KEYS)key] && KeyDown(key);
}

bool PlatformKeyboard::KeyReleased(int key) 
{
    return _keyStatesLast[(KEYBOARD_KEYS)key] != _keyStates[(KEYBOARD_KEYS)key] && KeyUp(key);
}