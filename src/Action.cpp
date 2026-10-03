
#include "Action.h"

bool Action::before(const Action& rhs) 
{ 
	if(this != &rhs)
		return 0 > (this->_actionTime - rhs._actionTime);
	else 
		return false; 
}

bool Action::after(const Action& rhs)
{ 
	if(this != &rhs)
		return 0 < (this->_actionTime - rhs._actionTime);
	else
		return false; 
}

bool Action::simultaneous(const Action& rhs) 
{
	if (this != &rhs)
		return 0 == (this->_actionTime - rhs._actionTime);
	else 
		return false; 
}

void Action::unassign(IKeyboard::KEY eKey)
{
	std::list<IKeyboard::KEY>::iterator itr = _inputAssignments.begin();
	for(;itr != _inputAssignments.end(); itr++)
	{
		if(eKey == (*itr))
		{
			_inputAssignments.erase(itr);
			break;
		}
	}
}

void Action::unassign(IGamepad::Button button)
{
	std::list<IGamepad::Button>::iterator itr = _gamepadButtonAssignments.begin();
	for (; itr != _gamepadButtonAssignments.end(); ++itr) {
		if (button == *itr) {
			_gamepadButtonAssignments.erase(itr);
			break;
		}
	}
}

void Action::unassignAxis(IGamepad::Axis axis, float threshold)
{
	std::list<AxisAssignment>::iterator itr = _gamepadAxisAssignments.begin();
	for (; itr != _gamepadAxisAssignments.end(); ++itr) {
		if (itr->axis == axis && itr->threshold == threshold) {
			_gamepadAxisAssignments.erase(itr);
			break;
		}
	}
}
