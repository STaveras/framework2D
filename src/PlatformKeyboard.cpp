
#include "PlatformKeyboard.h"

#include "Engine2D.h"

PlatformKeyboard::PlatformKeyboard(Window *window)
{
    // This is hideous but...
    auto func = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        InputInterface *input = Engine2D::GetInput();
        PlatformKeyboard *keyboard = (PlatformKeyboard*)input->GetKeyboard();
        keyboard->_onKeyEventHandler(window, key, scancode, action, mods);
    };
    glfwSetKeyCallback(window->getUnderlyingWindow(), func);
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