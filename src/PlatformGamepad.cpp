#include "PlatformGamepad.h"

#include <algorithm>

namespace {
int glfwButton(Gamepad::Button button)
{
	switch (button) {
	case Gamepad::Button::A: return GLFW_GAMEPAD_BUTTON_A;
	case Gamepad::Button::B: return GLFW_GAMEPAD_BUTTON_B;
	case Gamepad::Button::X: return GLFW_GAMEPAD_BUTTON_X;
	case Gamepad::Button::Y: return GLFW_GAMEPAD_BUTTON_Y;
	case Gamepad::Button::LeftBumper: return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
	case Gamepad::Button::RightBumper: return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
	case Gamepad::Button::Back: return GLFW_GAMEPAD_BUTTON_BACK;
	case Gamepad::Button::Start: return GLFW_GAMEPAD_BUTTON_START;
	case Gamepad::Button::Guide: return GLFW_GAMEPAD_BUTTON_GUIDE;
	case Gamepad::Button::LeftThumb: return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
	case Gamepad::Button::RightThumb: return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
	case Gamepad::Button::DpadUp: return GLFW_GAMEPAD_BUTTON_DPAD_UP;
	case Gamepad::Button::DpadRight: return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
	case Gamepad::Button::DpadDown: return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
	case Gamepad::Button::DpadLeft: return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
	}
	return -1;
}

int glfwAxis(Gamepad::Axis axis)
{
	switch (axis) {
	case Gamepad::Axis::LeftX: return GLFW_GAMEPAD_AXIS_LEFT_X;
	case Gamepad::Axis::LeftY: return GLFW_GAMEPAD_AXIS_LEFT_Y;
	case Gamepad::Axis::RightX: return GLFW_GAMEPAD_AXIS_RIGHT_X;
	case Gamepad::Axis::RightY: return GLFW_GAMEPAD_AXIS_RIGHT_Y;
	case Gamepad::Axis::LeftTrigger: return GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
	case Gamepad::Axis::RightTrigger: return GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
	}
	return -1;
}

bool axisIsDown(float value, float threshold)
{
	return threshold < 0.0f ? value <= threshold : value >= threshold;
}

float normalizedAxis(const GLFWgamepadstate& state, Gamepad::Axis axis)
{
	const int index = glfwAxis(axis);
	if (index < 0) {
		return 0.0f;
	}

	const float value = state.axes[index];
	if (axis == Gamepad::Axis::LeftTrigger || axis == Gamepad::Axis::RightTrigger) {
		return std::max(0.0f, std::min(1.0f, (value + 1.0f) * 0.5f));
	}
	return std::max(-1.0f, std::min(1.0f, value));
}
}

PlatformGamepad::PlatformGamepad(void)
{
	update();
}

int PlatformGamepad::_joystickForPad(int padIndex) const
{
	if (padIndex < 0 || (size_t)padIndex >= _connectedJoysticks.size()) {
		return -1;
	}
	return _connectedJoysticks[(size_t)padIndex];
}

bool PlatformGamepad::_axisDown(const JoystickState& joystick, Axis axis, float threshold)
{
	return joystick.stateValid && axisIsDown(normalizedAxis(joystick.state, axis), threshold);
}

bool PlatformGamepad::_axisWasDown(const JoystickState& joystick, Axis axis, float threshold)
{
	return joystick.previousStateValid && axisIsDown(normalizedAxis(joystick.previousState, axis), threshold);
}

int PlatformGamepad::getConnectedCount(void) const
{
	return (int)_connectedJoysticks.size();
}

bool PlatformGamepad::isConnected(int padIndex) const
{
	return _joystickForPad(padIndex) >= 0;
}

std::string PlatformGamepad::getName(int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0) {
		return std::string();
	}
	const char* name = glfwGetGamepadName(jid);
	return name ? std::string(name) : std::string();
}

std::string PlatformGamepad::getGUID(int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0) {
		return std::string();
	}
	const char* guid = glfwGetJoystickGUID(jid);
	return guid ? std::string(guid) : std::string();
}

bool PlatformGamepad::buttonDown(Button button, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	const int index = glfwButton(button);
	if (jid < 0 || index < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.buttons.down(index);
}

bool PlatformGamepad::buttonUp(Button button, int padIndex) const
{
	return !buttonDown(button, padIndex);
}

bool PlatformGamepad::buttonPressed(Button button, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	const int index = glfwButton(button);
	if (jid < 0 || index < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.buttons.pressed(index);
}

bool PlatformGamepad::buttonReleased(Button button, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	const int index = glfwButton(button);
	if (jid < 0 || index < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.buttons.released(index);
}

float PlatformGamepad::getAxis(Axis axis, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0 || glfwAxis(axis) < 0) {
		return 0.0f;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.stateValid ? normalizedAxis(joystick.state, axis) : 0.0f;
}

bool PlatformGamepad::axisDown(Axis axis, float threshold, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0 || glfwAxis(axis) < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return _axisDown(joystick, axis, threshold);
}

bool PlatformGamepad::axisPressed(Axis axis, float threshold, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0 || glfwAxis(axis) < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return ButtonEdge::pressed(_axisDown(joystick, axis, threshold), _axisWasDown(joystick, axis, threshold));
}

bool PlatformGamepad::axisReleased(Axis axis, float threshold, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0 || glfwAxis(axis) < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return ButtonEdge::released(_axisDown(joystick, axis, threshold), _axisWasDown(joystick, axis, threshold));
}

void PlatformGamepad::update(void)
{
	_connectedJoysticks.clear();
	for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
		JoystickState& joystick = _joysticks[(size_t)jid];
		joystick.previousState = joystick.state;
		joystick.previousStateValid = joystick.stateValid;

		joystick.connected = glfwJoystickPresent(jid) == GLFW_TRUE && glfwJoystickIsGamepad(jid) == GLFW_TRUE;
		joystick.stateValid = joystick.connected && glfwGetGamepadState(jid, &joystick.state) == GLFW_TRUE;
		if (!joystick.stateValid) {
			joystick.state = GLFWgamepadstate{};
		}
		joystick.buttons.beginFrame();
		for (int button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; ++button) {
			joystick.buttons.set(button, joystick.state.buttons[button] == GLFW_PRESS);
		}
		if (joystick.connected) {
			_connectedJoysticks.push_back(jid);
		}
	}
}
