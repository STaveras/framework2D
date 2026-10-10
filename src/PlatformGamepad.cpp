#include "PlatformGamepad.h"

#include "Types.h"

#include <algorithm>

namespace {
// The GLFW button for each Gamepad::Button, in Button order.
const int kGLFWButtons[Gamepad::kButtonCount] = {
	GLFW_GAMEPAD_BUTTON_A,
	GLFW_GAMEPAD_BUTTON_B,
	GLFW_GAMEPAD_BUTTON_X,
	GLFW_GAMEPAD_BUTTON_Y,
	GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,
	GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,
	GLFW_GAMEPAD_BUTTON_BACK,
	GLFW_GAMEPAD_BUTTON_START,
	GLFW_GAMEPAD_BUTTON_GUIDE,
	GLFW_GAMEPAD_BUTTON_LEFT_THUMB,
	GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,
	GLFW_GAMEPAD_BUTTON_DPAD_UP,
	GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
	GLFW_GAMEPAD_BUTTON_DPAD_DOWN,
	GLFW_GAMEPAD_BUTTON_DPAD_LEFT,
};

// The GLFW axis for each Gamepad::Axis, in Axis order.
const int kGLFWAxes[Gamepad::kAxisCount] = {
	GLFW_GAMEPAD_AXIS_LEFT_X,
	GLFW_GAMEPAD_AXIS_LEFT_Y,
	GLFW_GAMEPAD_AXIS_RIGHT_X,
	GLFW_GAMEPAD_AXIS_RIGHT_Y,
	GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,
	GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
};
}

PlatformGamepad::PlatformGamepad(void)
{
	update();
}

void PlatformGamepad::update(void)
{
	std::vector<Reading> readings;
	for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
		if (glfwJoystickPresent(jid) != GLFW_TRUE || glfwJoystickIsGamepad(jid) != GLFW_TRUE) {
			continue;
		}

		Reading& reading = readings.emplace_back();
		const char* name = glfwGetGamepadName(jid);
		const char* guid = glfwGetJoystickGUID(jid);
		reading.name = name ? name : "";
		reading.guid = guid ? guid : "";

		// A pad whose state can't be read this frame reads as released.
		GLFWgamepadstate state{};
		if (glfwGetGamepadState(jid, &state) != GLFW_TRUE) {
			continue;
		}
		for (int button = 0; button < kButtonCount; ++button) {
			reading.down[(size_t)button] = state.buttons[kGLFWButtons[button]] == GLFW_PRESS;
		}
		for (int axis = 0; axis < kAxisCount; ++axis) {
			const float value = state.axes[kGLFWAxes[axis]];
			const bool trigger = axis == (int)Axis::LeftTrigger || axis == (int)Axis::RightTrigger;
			// GLFW triggers rest at -1; ours rest at 0.
			reading.axes[(size_t)axis] = trigger
				? std::clamp((value + 1.0f) * 0.5f, 0.0f, 1.0f)
				: std::clamp(value, -1.0f, 1.0f);
		}
	}
	_setReadings(readings);
}
