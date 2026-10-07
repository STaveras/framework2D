#pragma once

#include "Types.h"
#include "Gamepad.h"
#include "ButtonState.h"

#include <array>
#include <vector>

class PlatformGamepad : public Gamepad
{
	struct JoystickState
	{
		bool connected = false;
		bool stateValid = false;
		bool previousStateValid = false;
		GLFWgamepadstate state{};
		GLFWgamepadstate previousState{};
		ButtonStateSet buttons{GLFW_GAMEPAD_BUTTON_LAST + 1};
	};

	std::array<JoystickState, GLFW_JOYSTICK_LAST + 1> _joysticks;
	std::vector<int> _connectedJoysticks;

	int _joystickForPad(int padIndex) const;
	// Axis past threshold this frame / last frame; false while the state is invalid.
	static bool _axisDown(const JoystickState& joystick, Axis axis, float threshold);
	static bool _axisWasDown(const JoystickState& joystick, Axis axis, float threshold);

public:
	PlatformGamepad(void);

	int getConnectedCount(void) const override;
	bool isConnected(int padIndex = 0) const override;
	std::string getName(int padIndex = 0) const override;
	std::string getGUID(int padIndex = 0) const override;

	bool buttonDown(Button button, int padIndex = 0) const override;
	bool buttonUp(Button button, int padIndex = 0) const override;
	bool buttonPressed(Button button, int padIndex = 0) const override;
	bool buttonReleased(Button button, int padIndex = 0) const override;

	float getAxis(Axis axis, int padIndex = 0) const override;
	bool axisDown(Axis axis, float threshold, int padIndex = 0) const override;
	bool axisPressed(Axis axis, float threshold, int padIndex = 0) const override;
	bool axisReleased(Axis axis, float threshold, int padIndex = 0) const override;

	void update(void) override;
};
