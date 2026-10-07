
#include "InputMap.h"
#include "InputTapeRecorder.h"
#include "ButtonState.h"

#include <algorithm>

void InputMap::addAction(Action action)
{
	Action* existingAction = this->getAction(action.getActionName());
	if (!existingAction) {
		_actions.push_back(action);
		return;
	}

	std::list<Keyboard::KEY>& existingAssignments = existingAction->getAssignments();
	for (Keyboard::KEY assignment : action.getAssignments()) {
		bool alreadyAssigned = false;
		for (Keyboard::KEY existingAssignment : existingAssignments) {
			if (existingAssignment == assignment) {
				alreadyAssigned = true;
				break;
			}
		}

		if (!alreadyAssigned) {
			existingAction->assign(assignment);
		}
	}

	for (Gamepad::Button button : action.getGamepadButtonAssignments()) {
		const auto& assignments = existingAction->getGamepadButtonAssignments();
		if (std::find(assignments.begin(), assignments.end(), button) == assignments.end()) {
			existingAction->assign(button);
		}
	}

	for (const Action::AxisAssignment& assignment : action.getGamepadAxisAssignments()) {
		const auto& assignments = existingAction->getGamepadAxisAssignments();
		if (std::find(assignments.begin(), assignments.end(), assignment) == assignments.end()) {
			existingAction->assignAxis(assignment.axis, assignment.threshold);
		}
	}
}

Action* InputMap::getAction(std::string actionName)
{
	std::list<Action>::iterator itr = _actions.begin();
	for (; itr != _actions.end(); itr++) {
		if (itr->getActionName() == actionName) {
			return &(*itr);
		}
	}

	return NULL;
}

void InputMap::removeAction(Action action)
{
	std::list<Action>::iterator itr = _actions.begin();
	for (; itr != _actions.end(); itr++)
	{
		if (action == (*itr)) {
			_actions.erase(itr);
			break;
		}
	}
}

bool InputMap::buttonPressed(Action* action)
{
	if (!_input || !action) {
		return false;
	}

	if (Keyboard* keyboard = _input->getKeyboard()) {
		for (Keyboard::KEY key : action->getAssignments()) {
			if (keyboard->keyPressed(key)) {
				return true;
			}
		}
	}

	if (Gamepad* gamepad = _input->getGamepad()) {
		for (Gamepad::Button button : action->getGamepadButtonAssignments()) {
			if (gamepad->buttonPressed(button, _padNumber)) {
				return true;
			}
		}
		for (const Action::AxisAssignment& assignment : action->getGamepadAxisAssignments()) {
			if (gamepad->axisPressed(assignment.axis, assignment.threshold, _padNumber)) {
				return true;
			}
		}
	}

	return false;
}

bool InputMap::buttonReleased(Action* action)
{
	if (!_input || !action) {
		return false;
	}

	if (Keyboard* keyboard = _input->getKeyboard()) {
		for (Keyboard::KEY key : action->getAssignments()) {
			if (keyboard->keyReleased(key)) {
				return true;
			}
		}
	}

	if (Gamepad* gamepad = _input->getGamepad()) {
		for (Gamepad::Button button : action->getGamepadButtonAssignments()) {
			if (gamepad->buttonReleased(button, _padNumber)) {
				return true;
			}
		}
		for (const Action::AxisAssignment& assignment : action->getGamepadAxisAssignments()) {
			if (gamepad->axisReleased(assignment.axis, assignment.threshold, _padNumber)) {
				return true;
			}
		}
	}

	return false;
}

bool InputMap::buttonDown(Action* action)
{
	if (!_input || !action) {
		return false;
	}

	if (Keyboard* keyboard = _input->getKeyboard()) {
		for (Keyboard::KEY key : action->getAssignments()) {
			if (keyboard->keyDown(key)) {
				return true;
			}
		}
	}

	if (Gamepad* gamepad = _input->getGamepad()) {
		for (Gamepad::Button button : action->getGamepadButtonAssignments()) {
			if (gamepad->buttonDown(button, _padNumber)) {
				return true;
			}
		}
		for (const Action::AxisAssignment& assignment : action->getGamepadAxisAssignments()) {
			if (gamepad->axisDown(assignment.axis, assignment.threshold, _padNumber)) {
				return true;
			}
		}
	}

	return false;
}

bool InputMap::buttonUp(Action* action)
{
	return action && !buttonDown(action);
}

void InputMap::update(float time)
{
	if (_input && _input->getGamepad()) {
		_connected = _input->getGamepad()->isConnected(_padNumber);
	}
	_elapsedTime += time;
	const uint64_t simulationTick = Engine2D::getSimulationTick();
	InputTapeRecorder::onControllerTickStart(this, simulationTick, _elapsedTime);

	for (Action& action : this->getActions()) {
		const std::string actionName = action.getActionName();
		const bool replayControlled = InputTapeRecorder::isReplayControlledAction(actionName);
		const bool previousActive = action.isActive();

		bool isDown = false;
		bool isUp = false;
		bool isPressed = false;
		bool isReleased = false;

		if (replayControlled) {
			const bool replayState = InputTapeRecorder::getReplayActionState(actionName, previousActive);
			isDown = replayState;
			isUp = !replayState;
			isPressed = ButtonEdge::pressed(replayState, previousActive);
			isReleased = ButtonEdge::released(replayState, previousActive);
		}
		else {
			isDown = this->buttonDown(&action);
			isUp = !isDown;
			isPressed = ButtonEdge::pressed(isDown, previousActive);
			isReleased = ButtonEdge::released(isDown, previousActive);
		}

		if (isDown) {
			action.setActive(true);
			_eventSystem->sendEvent<InputEvent>(
				InputEvent(EVT_KEYDOWN, this, _elapsedTime, actionName),
				nullptr,
				Event::event_priority_immediate);
			InputTapeRecorder::onControllerActionEvent(simulationTick, _elapsedTime, actionName, EVT_KEYDOWN, true);
		}
		else if (isUp) {
			action.setActive(false);
			_eventSystem->sendEvent<InputEvent>(
				InputEvent(EVT_KEYUP, this, _elapsedTime, actionName),
				nullptr,
				Event::event_priority_immediate);
			InputTapeRecorder::onControllerActionEvent(simulationTick, _elapsedTime, actionName, EVT_KEYUP, false);
		}

		if (isPressed)
		{
			action.setActive(true);
			_eventSystem->sendEvent<InputEvent>(
				InputEvent(EVT_KEYPRESSED, this, _elapsedTime, actionName),
				nullptr,
				Event::event_priority_immediate);
			InputTapeRecorder::onControllerActionEvent(simulationTick, _elapsedTime, actionName, EVT_KEYPRESSED, true);
		}
		else if (isReleased)
		{
			action.setActive(false);
			_eventSystem->sendEvent<InputEvent>(
				InputEvent(EVT_KEYRELEASED, this, _elapsedTime, actionName),
				nullptr,
				Event::event_priority_immediate);
			InputTapeRecorder::onControllerActionEvent(simulationTick, _elapsedTime, actionName, EVT_KEYRELEASED, false);
		}

		InputTapeRecorder::recordActionSnapshot(simulationTick, actionName, action.isActive());
	}
}
