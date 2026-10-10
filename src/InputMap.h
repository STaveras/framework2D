// File: InputMap.h
// A set of named Actions and the inputs bound to them. update() samples every
// action once per tick (from the input devices, or from the input tape during
// a replay), and game code reads actions by name:
//
//   map.bind("JUMP").key(Key::Space).button(Gamepad::Button::A);
//   if (map.pressed("JUMP")) ...
//
// Names nothing is bound to read as released.

#pragma once

#include "Action.h"

#include <deque>
#include <string>

class IInput;

class InputMap
{
	IInput* _input = nullptr;
	int _pad = 0;
	// A deque keeps the references bind() returns valid as actions are added.
	std::deque<Action> _actions;

	Action* _find(const std::string& name);
	bool _readDevices(const Action& action) const;

public:
	explicit InputMap(IInput* input = nullptr) : _input(input) {}

	IInput* getInput(void) const { return _input; }
	void setInput(IInput* input) { _input = input; }

	// The gamepad this map reads (0 is the first connected pad).
	int getPadIndex(void) const { return _pad; }
	void setPadIndex(int pad) { _pad = pad; }

	// The named action, created on first use, to bind inputs to.
	Action& bind(const std::string& name);

	bool down(const std::string& name) const;
	bool pressed(const std::string& name) const;
	bool released(const std::string& name) const;

	const Action* find(const std::string& name) const;
	// In the order they were first bound.
	const std::deque<Action>& getActions(void) const { return _actions; }

	// Hold or release an action from code rather than a device (tests,
	// scripted input). Takes effect at the next update().
	void drive(const std::string& name, bool down);

	// Sample every action for this tick, and record the changes to the input
	// tape when recording.
	void update(void);
};
