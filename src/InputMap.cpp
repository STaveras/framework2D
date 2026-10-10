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

void InputMap::update(float time)
{
	_elapsedTime += time;
	const uint64_t tick = Engine2D::getSimulationTick();
	InputTapeRecorder::onControllerTickStart(this, tick, _elapsedTime);

	for (Action& action : _actions) {
		const std::string& name = action.getName();
		action._wasDown = action._down;
		action._down = InputTapeRecorder::isReplayControlledAction(name)
			? InputTapeRecorder::getReplayActionState(name, action._wasDown)
			: (action._driven || _readDevices(action));

		InputTapeRecorder::onControllerActionEvent(tick, _elapsedTime, name,
			action._down ? "EVT_KEYDOWN" : "EVT_KEYUP", action._down);
		if (action.pressed()) {
			InputTapeRecorder::onControllerActionEvent(tick, _elapsedTime, name, "EVT_KEYPRESSED", true);
		}
		else if (action.released()) {
			InputTapeRecorder::onControllerActionEvent(tick, _elapsedTime, name, "EVT_KEYRELEASED", false);
		}
	}
}
