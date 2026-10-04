#include "PlatformGamepad.h"

#include <algorithm>

namespace {
int glfwButton(IGamepad::Button button)
{
	switch (button) {
	case IGamepad::Button::A: return GLFW_GAMEPAD_BUTTON_A;
	case IGamepad::Button::B: return GLFW_GAMEPAD_BUTTON_B;
	case IGamepad::Button::X: return GLFW_GAMEPAD_BUTTON_X;
	case IGamepad::Button::Y: return GLFW_GAMEPAD_BUTTON_Y;
	case IGamepad::Button::LeftBumper: return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
	case IGamepad::Button::RightBumper: return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
	case IGamepad::Button::Back: return GLFW_GAMEPAD_BUTTON_BACK;
	case IGamepad::Button::Start: return GLFW_GAMEPAD_BUTTON_START;
	case IGamepad::Button::Guide: return GLFW_GAMEPAD_BUTTON_GUIDE;
	case IGamepad::Button::LeftThumb: return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
	case IGamepad::Button::RightThumb: return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
	case IGamepad::Button::DpadUp: return GLFW_GAMEPAD_BUTTON_DPAD_UP;
	case IGamepad::Button::DpadRight: return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
	case IGamepad::Button::DpadDown: return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
	case IGamepad::Button::DpadLeft: return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
	}
	return -1;
}

int glfwAxis(IGamepad::Axis axis)
{
	switch (axis) {
	case IGamepad::Axis::LeftX: return GLFW_GAMEPAD_AXIS_LEFT_X;
	case IGamepad::Axis::LeftY: return GLFW_GAMEPAD_AXIS_LEFT_Y;
	case IGamepad::Axis::RightX: return GLFW_GAMEPAD_AXIS_RIGHT_X;
	case IGamepad::Axis::RightY: return GLFW_GAMEPAD_AXIS_RIGHT_Y;
	case IGamepad::Axis::LeftTrigger: return GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
	case IGamepad::Axis::RightTrigger: return GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
	}
	return -1;
}

bool axisIsDown(float value, float threshold)
{
	return threshold < 0.0f ? value <= threshold : value >= threshold;
}

float normalizedAxis(const GLFWgamepadstate& state, IGamepad::Axis axis)
{
	const int index = glfwAxis(axis);
	if (index < 0) {
		return 0.0f;
	}

	const float value = state.axes[index];
	if (axis == IGamepad::Axis::LeftTrigger || axis == IGamepad::Axis::RightTrigger) {
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
	return joystick.stateValid && joystick.state.buttons[index] == GLFW_PRESS;
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
	return joystick.stateValid && joystick.state.buttons[index] == GLFW_PRESS &&
		(!joystick.previousStateValid || joystick.previousState.buttons[index] != GLFW_PRESS);
}

bool PlatformGamepad::buttonReleased(Button button, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	const int index = glfwButton(button);
	if (jid < 0 || index < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.previousStateValid && joystick.previousState.buttons[index] == GLFW_PRESS &&
		(!joystick.stateValid || joystick.state.buttons[index] != GLFW_PRESS);
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
	return joystick.stateValid && axisIsDown(normalizedAxis(joystick.state, axis), threshold);
}

bool PlatformGamepad::axisPressed(Axis axis, float threshold, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0 || glfwAxis(axis) < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.stateValid && axisIsDown(normalizedAxis(joystick.state, axis), threshold) &&
		(!joystick.previousStateValid || !axisIsDown(normalizedAxis(joystick.previousState, axis), threshold));
}

bool PlatformGamepad::axisReleased(Axis axis, float threshold, int padIndex) const
{
	const int jid = _joystickForPad(padIndex);
	if (jid < 0 || glfwAxis(axis) < 0) {
		return false;
	}
	const JoystickState& joystick = _joysticks[(size_t)jid];
	return joystick.previousStateValid && axisIsDown(normalizedAxis(joystick.previousState, axis), threshold) &&
		(!joystick.stateValid || !axisIsDown(normalizedAxis(joystick.state, axis), threshold));
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
		if (joystick.connected) {
			_connectedJoysticks.push_back(jid);
		}
	}
}
