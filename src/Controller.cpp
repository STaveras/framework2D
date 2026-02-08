
#include "Controller.h"

void Controller::addAction(Action action)
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
}

Action* Controller::getAction(std::string actionName)
{
	std::list<Action>::iterator itr = _actions.begin();
	for (; itr != _actions.end(); itr++) {
		if (itr->getActionName() == actionName) {
			return &(*itr);
		}
	}

	return NULL;
}

void Controller::removeAction(Action action)
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

bool Controller::buttonPressed(Action* action)
{
	if (_input)
	{
		std::list<Keyboard::KEY>::const_iterator itr2 = action->getAssignments().begin();

		for (; itr2 != action->getAssignments().end(); itr2++)
		{
			if (_input->getKeyboard()->keyPressed((*itr2))) {
				return true;
			}
		}
	}

	return false;
}

bool Controller::buttonReleased(Action* action)
{
	if (_input)
	{
		std::list<Keyboard::KEY>::const_iterator itr2 = action->getAssignments().begin();

		for (; itr2 != action->getAssignments().end(); itr2++)
		{
			if (_input->getKeyboard()->keyReleased((*itr2))) {
				return true;
			}
		}
	}

	return false;
}

bool Controller::buttonDown(Action* action)
{
	if (_input)
	{
		std::list<Keyboard::KEY>::const_iterator itr2 = action->getAssignments().begin();
		for (; itr2 != action->getAssignments().end(); itr2++)
		{
			if (_input->getKeyboard()->keyDown(*itr2)) {
				return true;
			}
		}
	}

	return false;
}

bool Controller::buttonUp(Action* action)
{
	if (_input)
	{
		std::list<Keyboard::KEY>::const_iterator itr2 = action->getAssignments().begin();
		for (; itr2 != action->getAssignments().end(); itr2++)
		{
			if (_input->getKeyboard()->keyUp(*itr2)) {
				return true;
			}
		}
	}

	return false;
}

void Controller::update(float time)
{
	_elapsedTime += time;

	for (Action& action : this->getActions()) {

		if (this->buttonDown(&action)) {
			action.setActive(true);
			_eventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYDOWN, this, _elapsedTime, action.getActionName()));
		}
		else if (this->buttonUp(&action)) {
			action.setActive(false);
			_eventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYUP, this, _elapsedTime, action.getActionName()));
		}

		if (this->buttonPressed(&action))
		{
			action.setActive(true);
			_eventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYPRESSED, this, _elapsedTime, action.getActionName()));
		}
		else if (this->buttonReleased(&action))
		{
			action.setActive(false);
			_eventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYRELEASED, this, _elapsedTime, action.getActionName()));
		}
	}
}
