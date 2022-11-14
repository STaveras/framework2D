
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

void Action::unassign(KEYBOARD_KEYS eKey)
{
	std::list<KEYBOARD_KEYS>::iterator itr = _inputAssignments.begin();
	for(;itr != _inputAssignments.end(); itr++)
	{
		if(eKey == (*itr))
		{
			_inputAssignments.erase(itr);
			break;
		}
	}
}