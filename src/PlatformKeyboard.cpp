
#include "PlatformKeyboard.h"

#include "Engine2D.h"

PlatformKeyboard::PlatformKeyboard(Window *window)
{
    _window = window ? window->getUnderlyingWindow() : nullptr;
    _keyStates.assign(GLFW_KEY_LAST + 1, 0);
    _keyStatesLast.assign(GLFW_KEY_LAST + 1, 0);
}

void PlatformKeyboard::_onKeyEventHandler(GLFWwindow *window, int key, int scancode, int action, int mods)
{





    // Intentionally unused: we poll key states each frame in update().
}

bool PlatformKeyboard::keyDown(KEY key)
{
    if (key < 0 || key > GLFW_KEY_LAST) {
        return false;
    }
    return _keyStates[(size_t)key] != 0;
}

bool PlatformKeyboard::keyUp(KEY key)
{
    return !keyDown(key);
}

bool PlatformKeyboard::keyPressed(KEY key)
{
    if (key < 0 || key > GLFW_KEY_LAST) {
        return false;
    }
    size_t k = (size_t)key;
    return _keyStates[k] && !_keyStatesLast[k];
}

bool PlatformKeyboard::keyReleased(KEY key)
{
    if (key < 0 || key > GLFW_KEY_LAST) {
        return false;
    }
    size_t k = (size_t)key;
    return !_keyStates[k] && _keyStatesLast[k];
}

void PlatformKeyboard::update(void)
{
    if (!_window) {
        return;
    }

    _keyStatesLast = _keyStates;

    for (int key = 0; key <= GLFW_KEY_LAST; ++key) {
        int state = glfwGetKey(_window, key);
        _keyStates[(size_t)key] = (state == GLFW_PRESS || state == GLFW_REPEAT) ? 1 : 0;
    }
}