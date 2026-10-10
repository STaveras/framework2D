// PlatformGamepad.h
// Gamepads through GLFW. Only joysticks with a GLFW gamepad mapping count;
// pads are numbered in GLFW joystick-ID order.

#pragma once

#include "Gamepad.h"

class PlatformGamepad : public Gamepad
{
public:
	PlatformGamepad(void);

	void update(void) override;
};
