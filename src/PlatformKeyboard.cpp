
#include "PlatformKeyboard.h"

#include "Engine2D.h"

PlatformKeyboard::PlatformKeyboard(Window *window)
{
    _owner = window;
    _window = window ? window->getUnderlyingWindow() : nullptr;
    _keyStates.assign(GLFW_KEY_LAST + 1, 0);
    _keyStatesLast.assign(GLFW_KEY_LAST + 1, 0);
    _keyPressLatch.assign(GLFW_KEY_LAST + 1, 0);

    if (_owner && _window) {
        _owner->setKeyEventHandler([this](int key, int scancode, int action, int mods) {
            _onKeyEventHandler(_window, key, scancode, action, mods);
        });
    }
}

PlatformKeyboard::~PlatformKeyboard(void)
{
    if (_owner && _window) {
        _owner->setKeyEventHandler(nullptr);
    }
}

void PlatformKeyboard::_onKeyEventHandler(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key < 0 || key > GLFW_KEY_LAST) {
        return;
    }

    // Latch presses until the next update() so a press and release that both
    // land inside one glfwPollEvents() still reads as down for a frame
    if (action == GLFW_PRESS) {
        _keyPressLatch[(size_t)key] = 1;
    }
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
        bool held = (state == GLFW_PRESS || state == GLFW_REPEAT);
        _keyStates[(size_t)key] = (held || _keyPressLatch[(size_t)key]) ? 1 : 0;
        _keyPressLatch[(size_t)key] = 0;
    }
}