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
};