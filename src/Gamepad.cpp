#include "Gamepad.h"

namespace {
bool axisIsDown(float value, float threshold)
{
	return threshold < 0.0f ? value <= threshold : value >= threshold;
}
}

const Gamepad::Pad* Gamepad::_pad(int pad) const
{
	if (pad < 0 || (size_t)pad >= _connected) {
		return nullptr;
	}
	return &_pads[(size_t)pad];
}

std::string Gamepad::getName(int pad) const
{
	const Pad* state = _pad(pad);
	return state ? state->name : std::string();
}

std::string Gamepad::getGUID(int pad) const
{
	const Pad* state = _pad(pad);
	return state ? state->guid : std::string();
}

bool Gamepad::down(Button button, int pad) const
{
	const Pad* state = _pad(pad);
	return state && state->buttons.down((int)button);
}

bool Gamepad::pressed(Button button, int pad) const
{
	const Pad* state = _pad(pad);
	return state && state->buttons.pressed((int)button);
}

bool Gamepad::released(Button button, int pad) const
{
	const Pad* state = _pad(pad);
	return state && state->buttons.released((int)button);
}

float Gamepad::axis(Axis axis, int pad) const
{
	const Pad* state = _pad(pad);
	return (state && axis != Axis::Count) ? state->axes[(size_t)axis] : 0.0f;
}

bool Gamepad::axisDown(Axis axis, float threshold, int pad) const
{
	const Pad* state = _pad(pad);
	return state && axis != Axis::Count && axisIsDown(state->axes[(size_t)axis], threshold);
}

bool Gamepad::axisPressed(Axis axis, float threshold, int pad) const
{
	const Pad* state = _pad(pad);
	return state && axis != Axis::Count &&
		ButtonEdge::pressed(axisIsDown(state->axes[(size_t)axis], threshold),
			axisIsDown(state->previousAxes[(size_t)axis], threshold));
}

bool Gamepad::axisReleased(Axis axis, float threshold, int pad) const
{
	const Pad* state = _pad(pad);
	return state && axis != Axis::Count &&
		ButtonEdge::released(axisIsDown(state->axes[(size_t)axis], threshold),
			axisIsDown(state->previousAxes[(size_t)axis], threshold));
}

void Gamepad::_setReadings(const std::vector<Reading>& readings)
{
	if (_pads.size() < readings.size()) {
		_pads.resize(readings.size());
	}
	for (size_t i = 0; i < _pads.size(); ++i) {
		Pad& pad = _pads[i];
		const Reading* reading = (i < readings.size()) ? &readings[i] : nullptr;
		pad.name = reading ? reading->name : std::string();
		pad.guid = reading ? reading->guid : std::string();
		pad.buttons.beginFrame();
		for (int button = 0; button < kButtonCount; ++button) {
			if (reading && reading->tapped[(size_t)button]) {
				pad.buttons.latchPress(button);
			}
			pad.buttons.set(button, reading && reading->down[(size_t)button]);
		}
		pad.previousAxes = pad.axes;
		if (reading) {
			pad.axes = reading->axes;
		}
		else {
			pad.axes.fill(0.0f);
		}
	}
	_connected = readings.size();
}
