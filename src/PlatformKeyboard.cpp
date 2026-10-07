
#include "PlatformKeyboard.h"

#include "Engine2D.h"

PlatformKeyboard::PlatformKeyboard(Window *window)
{
    _owner = window;
    _window = window ? window->getUnderlyingWindow() : nullptr;
    _keys.resize(GLFW_KEY_LAST + 1);

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
        _keys.latchPress(key);
    }
}

bool PlatformKeyboard::keyDown(KEY key)
{
    return _keys.down(key);
}

bool PlatformKeyboard::keyUp(KEY key)
{
    return _keys.up(key);
}

bool PlatformKeyboard::keyPressed(KEY key)
{
    return _keys.pressed(key);
}

bool PlatformKeyboard::keyReleased(KEY key)
{
    return _keys.released(key);
}

void PlatformKeyboard::update(void)
{
    if (!_window) {
        return;
    }

    _keys.beginFrame();

    for (int key = 0; key <= GLFW_KEY_LAST; ++key) {
        int state = glfwGetKey(_window, key);
        _keys.set(key, state == GLFW_PRESS || state == GLFW_REPEAT);
    }
}