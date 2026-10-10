// Action.h
// A named game action ("JUMP", "PAUSE") and the inputs bound to it: keys,
// gamepad buttons, and gamepad axes past a threshold. Any one bound input
// holds the action. InputMap owns actions and samples them once per tick.

#pragma once

#include "ButtonState.h"
#include "Gamepad.h"
#include "Key.h"

#include <string>
#include <vector>

class Action
{
public:
	struct AxisBinding
	{
		Gamepad::Axis axis;
		// Held past a positive threshold above it, a negative one below it.
		float threshold;
	};

	explicit Action(std::string name) : _name(std::move(name)) {}

	const std::string& getName(void) const { return _name; }

	// Bind another input. Chainable:
	//   map.bind("JUMP").key(Key::Space).button(Gamepad::Button::A);
	Action& key(Key key);
	Action& button(Gamepad::Button button);
	Action& axis(Gamepad::Axis axis, float threshold);

	const std::vector<Key>& getKeys(void) const { return _keys; }
	const std::vector<Gamepad::Button>& getButtons(void) const { return _buttons; }
	const std::vector<AxisBinding>& getAxes(void) const { return _axes; }

	// State as of the last InputMap::update()
	bool down(void) const { return _down; }
	bool pressed(void) const { return ButtonEdge::pressed(_down, _wasDown); }
	bool released(void) const { return ButtonEdge::released(_down, _wasDown); }

private:
	friend class InputMap;

	std::string _name;
	std::vector<Key> _keys;
	std::vector<Gamepad::Button> _buttons;
	std::vector<AxisBinding> _axes;

	bool _down = false;
	bool _wasDown = false;
	bool _driven = false; // held by InputMap::drive()
};
