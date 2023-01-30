
#include "Controller.h"

Action* Controller::getAction(std::string actionName)
{
	std::list<Action>::iterator itr = _actions.begin();
	for (;itr != _actions.end(); itr++) {
		if (itr->getActionName() == actionName) {
			return &(*itr);
		}
	}

	return NULL;
}

void Controller::removeAction(Action action)
{
	std::list<Action>::iterator itr = _actions.begin();
	for (;itr != _actions.end(); itr++)
	{
		if (action == (*itr)) {
			_actions.erase(itr);
			break;
		}
	}
}

bool Controller::buttonPressed(std::string actionName)
{
	if (_input)
	{
		std::list<Action>::iterator itr = _actions.begin();
		for (;itr != _actions.end(); itr++)
		{
			if (actionName == itr->getActionName())
			{
				std::list<Keyboard::KEY>::const_iterator itr2 = itr->getAssignments().begin();

				for (; itr2 != itr->getAssignments().end(); itr2++)
				{
					if (_input->getKeyboard()->keyPressed((*itr2)))
					{
						return true;
					}
				}

				break;
			}
		}
	}

	return false;
}

bool Controller::buttonReleased(std::string actionName)
{
	if (_input)
	{
		std::list<Action>::iterator itr = _actions.begin();
		for (;itr != _actions.end(); itr++)
		{
			if (actionName == itr->getActionName())
			{
				std::list<Keyboard::KEY>::const_iterator itr2 = itr->getAssignments().begin();

				for (; itr2 != itr->getAssignments().end(); itr2++)
				{
					if (_input->getKeyboard()->keyReleased((*itr2)))
					{
						return true;
					}
				}

				break;
			}
		}
	}

	return false;
}