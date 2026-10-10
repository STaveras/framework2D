#include "InputMap.h"

#include "Engine2D.h"
#include "IInput.h"
#include "InputTapeRecorder.h"

Action* InputMap::_find(const std::string& name)
{
	for (Action& action : _actions) {
		if (action.getName() == name) {
			return &action;
		}
	}
	return nullptr;
}

const Action* InputMap::find(const std::string& name) const
{
	for (const Action& action : _actions) {
		if (action.getName() == name) {
			return &action;
		}
	}
	return nullptr;
}

Action& InputMap::bind(const std::string& name)
{
	if (Action* action = _find(name)) {
		return *action;
	}
	return _actions.emplace_back(name);
}

bool InputMap::down(const std::string& name) const
{
	const Action* action = find(name);
	return action && action->down();
}

bool InputMap::pressed(const std::string& name) const
{
	const Action* action = find(name);
	return action && action->pressed();
}

bool InputMap::released(const std::string& name) const
{
	const Action* action = find(name);
	return action && action->released();
}

void InputMap::drive(const std::string& name, bool down)
{
	bind(name)._driven = down;
}

bool InputMap::_readDevices(const Action& action) const
{
	if (!_input) {
		return false;
	}

	if (const Keyboard* keyboard = _input->getKeyboard()) {
		for (Key key : action.getKeys()) {
			if (keyboard->down(key)) {
				return true;
			}
		}
	}

	if (const Gamepad* gamepad = _input->getGamepad()) {
		for (Gamepad::Button button : action.getButtons()) {
			if (gamepad->down(button, _pad)) {
				return true;
			}
		}
		for (const Action::AxisBinding& binding : action.getAxes()) {
			if (gamepad->axisDown(binding.axis, binding.threshold, _pad)) {
				return true;
			}
		}
	}

	return false;
}

void InputMap::update(void)
{
	const uint64_t tick = Engine2D::getSimulationTick();
	const bool replaying = InputTapeRecorder::isReplaying();
	if (replaying) {
		InputTapeRecorder::beginTick(tick);
	}

	for (Action& action : _actions) {
		action._wasDown = action._down;
		action._down = replaying
			? InputTapeRecorder::replayState(action.getName())
			: (action._driven || _readDevices(action));
		if (action._down != action._wasDown) {
			InputTapeRecorder::recordChange(tick, action.getName(), action._down);
		}
	}
}
