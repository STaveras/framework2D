// Gamepad.h
// This frame's state of every connected gamepad: buttons with pressed/released
// edges, and stick/trigger axes. A platform backend reads its pads in update()
// and hands them over as Readings; the queries are shared by every backend.
// Pads are numbered 0.. in the order the backend lists them.

#pragma once

#include "ButtonState.h"

#include <array>
#include <string>
#include <vector>

class Gamepad
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
		DpadLeft,
		Count
	};

	enum class Axis
	{
		LeftX,
		LeftY,
		RightX,
		RightY,
		LeftTrigger,
		RightTrigger,
		Count
	};

	static constexpr int kButtonCount = (int)Button::Count;
	static constexpr int kAxisCount = (int)Axis::Count;

	virtual ~Gamepad(void) = default;

	int getConnectedCount(void) const { return (int)_connected; }
	bool isConnected(int pad = 0) const { return _pad(pad) != nullptr; }
	std::string getName(int pad = 0) const;
	std::string getGUID(int pad = 0) const;

	bool down(Button button, int pad = 0) const;
	bool up(Button button, int pad = 0) const { return !down(button, pad); }
	bool pressed(Button button, int pad = 0) const;
	bool released(Button button, int pad = 0) const;

	// Sticks range from -1 to 1 (+y is down); triggers from 0 (released) to 1.
	float axis(Axis axis, int pad = 0) const;
	// An axis is held past a positive threshold above it, a negative one below it.
	bool axisDown(Axis axis, float threshold, int pad = 0) const;
	bool axisPressed(Axis axis, float threshold, int pad = 0) const;
	bool axisReleased(Axis axis, float threshold, int pad = 0) const;

	virtual void update(void) = 0;

protected:
	// One connected pad as a backend reads it this frame.
	struct Reading
	{
		std::string name;
		std::string guid;
		std::array<bool, kButtonCount> down{};
		// Pressed since the last update, so a tap shorter than a frame still registers.
		std::array<bool, kButtonCount> tapped{};
		std::array<float, kAxisCount> axes{};
	};

	// Store this frame's readings (the connected pads, in order) and advance
	// the edges. Pads past the readings read as released, so one that comes
	// back starts clean.
	void _setReadings(const std::vector<Reading>& readings);

private:
	struct Pad
	{
		std::string name;
		std::string guid;
		ButtonStateSet buttons{ (size_t)kButtonCount };
		std::array<float, kAxisCount> axes{};
		std::array<float, kAxisCount> previousAxes{};
	};

	std::vector<Pad> _pads;
	size_t _connected = 0;

	const Pad* _pad(int pad) const;
};
