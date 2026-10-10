#include "Action.h"

#include <algorithm>

Action& Action::key(Key key)
{
	if (std::find(_keys.begin(), _keys.end(), key) == _keys.end()) {
		_keys.push_back(key);
	}
	return *this;
}

Action& Action::button(Gamepad::Button button)
{
	if (std::find(_buttons.begin(), _buttons.end(), button) == _buttons.end()) {
		_buttons.push_back(button);
	}
	return *this;
}

Action& Action::axis(Gamepad::Axis axis, float threshold)
{
	const bool bound = std::any_of(_axes.begin(), _axes.end(), [&](const AxisBinding& binding) {
		return binding.axis == axis && binding.threshold == threshold;
	});
	if (!bound) {
		_axes.push_back({ axis, threshold });
	}
	return *this;
}
