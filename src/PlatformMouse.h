// PlatformMouse.h
// Mouse input from a GLFW window. Each frame it reads the cursor position
// (client pixels, origin top-left, negative or past the client size once the
// cursor leaves the window) and the button state, and hides the OS cursor
// while it is over the window.

#pragma once

#include "Mouse.h"
#include "Types.h"

class Window;

class PlatformMouse : public Mouse
{
	GLFWwindow* _window;
	bool _cursorHidden;

public:
	explicit PlatformMouse(Window* window);
	~PlatformMouse(void) override;

	void update(void) override;
};
