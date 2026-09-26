#pragma once

#include <string>

namespace framework {

class IGamepad
{
public:
	enum class Button
	{
		// DualShock Cross, Circle, Square, and Triangle map to A, B, X, and Y.
		A,
		B,
		X,
		Y,
		LeftBumper,
		RightBumper,
		Back,
		Start,
		Guide,
		LeftThumb,
		RightThumb,
		DpadUp,
		DpadRight,
		DpadDown,
		DpadLeft
	};

	enum class Axis
	{
		LeftX,
		LeftY,
		RightX,
		RightY,
		LeftTrigger,
		RightTrigger
	};

	virtual ~IGamepad() = default;

	// Only GLFW-mapped gamepads are included; indices follow GLFW joystick-ID order.
	virtual int getConnectedCount(void) const = 0;
	virtual bool isConnected(int padIndex = 0) const = 0;
	virtual std::string getName(int padIndex = 0) const = 0;
	virtual std::string getGUID(int padIndex = 0) const = 0;

	virtual bool buttonDown(Button button, int padIndex = 0) const = 0;
	virtual bool buttonUp(Button button, int padIndex = 0) const = 0;
	virtual bool buttonPressed(Button button, int padIndex = 0) const = 0;
	virtual bool buttonReleased(Button button, int padIndex = 0) const = 0;

	// Stick axes range from -1 to 1. Trigger axes range from 0 (released) to 1.
	virtual float getAxis(Axis axis, int padIndex = 0) const = 0;
	virtual bool axisDown(Axis axis, float threshold, int padIndex = 0) const = 0;
	virtual bool axisPressed(Axis axis, float threshold, int padIndex = 0) const = 0;
	virtual bool axisReleased(Axis axis, float threshold, int padIndex = 0) const = 0;

	virtual void update(void) = 0;
};

using Gamepad = IGamepad;

}

using framework::Gamepad;
