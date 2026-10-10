// PlatformInput.h
// Input from a GLFW window: keyboard, mouse and GLFW-mapped gamepads.

#pragma once

#include "IInput.h"

class Window;

class PlatformInput : public IInput
{
public:
	explicit PlatformInput(Window* window);

	void initialize(void) override;
};
