
#include "Controller.h"

Action* Controller::getActions(ActionName eKey)
{
	std::list<Action>::iterator itr = _actions.begin();
	for (;itr != _actions.end(); itr++)
	{
		if (itr->getActionName() == eKey)
		{
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
		if(action == (*itr))
		{
			_actions.erase(itr);
			break;
		}
	}
}

bool Controller::buttonPressed(ActionName actionName)
{
	if (m_pInput)
	{
		std::list<Action>::iterator itr = _actions.begin();
		for (;itr != _actions.end(); itr++)
		{
			if (actionName == itr->getActionName())
			{
				std::list<KEYBOARD_KEYS>::const_iterator itr2 = itr->getAssignments().begin();

				for (; itr2 != itr->getAssignments().end(); itr2++)
				{
					if (m_pInput->GetKeyboard()->KeyPressed((*itr2)))
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

bool Controller::buttonReleased(ActionName actionName)
{
	if(m_pInput)
	{
		std::list<Action>::iterator itr = _actions.begin();
		for(;itr != _actions.end(); itr++)
		{
			if(actionName == itr->getActionName())
			{
				std::list<KEYBOARD_KEYS>::const_iterator itr2 = itr->getAssignments().begin();

				for(; itr2 != itr->getAssignments().end(); itr2++)
				{
					if(m_pInput->GetKeyboard()->KeyReleased((*itr2)))
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