
#include "PlatformKeyboard.h"

#include "Engine2D.h"

PlatformKeyboard::PlatformKeyboard(Window *window): _onKeyEvent(NULL)
{
    // This is hideous but...
    auto func = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        InputInterface *input = Engine2D::getInput();
        PlatformKeyboard *keyboard = (PlatformKeyboard*)input->getKeyboard();
        keyboard->_onKeyEventHandler(window, key, scancode, action, mods);
    };
    glfwSetKeyCallback(window->getUnderlyingWindow(), func);
    // ..it works. But I'm concerned as to how it works, in regards to memory management 
    // Are lambda funcs passed as a stack object?
}

void PlatformKeyboard::_onKeyEventHandler(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    _keyStatesLast[(KEY)key] = _keyStates[(KEY)key];
    _keyStates[(KEY)key] = (action == GLFW_RELEASE) ? false : true;
}

bool PlatformKeyboard::keyDown(KEY key)
{
    return _keyStates[key];
}

bool PlatformKeyboard::keyUp(KEY key)
{
    return !_keyStates[key];
}

bool PlatformKeyboard::keyPressed(KEY key)
{
    return _keyStatesLast[key] != _keyStates[key] && keyDown(key);
}

bool PlatformKeyboard::keyReleased(KEY key)
{
    return _keyStatesLast[key] != _keyStates[key] && keyUp(key);
}