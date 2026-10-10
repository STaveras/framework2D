// PlatformKeyboard.h
// Keyboard input from a GLFW window.

#pragma once

#include "Keyboard.h"
#include "Types.h"

class Window;

class PlatformKeyboard : public Keyboard
{
	Window* _owner;
	GLFWwindow* _window;

	void _onKeyEvent(int key, int action);

public:
	explicit PlatformKeyboard(Window* window);
	~PlatformKeyboard(void) override;

	void update(void) override;
};
