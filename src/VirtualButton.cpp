
#include "VirtualButton.h"

bool VirtualButton::Before(const VirtualButton& rhs) 
{ 
	if(this != &rhs)
	{
		return (this->m_fTimeStamp - rhs.m_fTimeStamp) < 0;
	} 
	else 
		return false; 
}

bool VirtualButton::After(const VirtualButton& rhs)
{ 
	if(this != &rhs) 
	{
		return (this->m_fTimeStamp - rhs.m_fTimeStamp) > 0;
	} 
	else
		return false; 
}

bool VirtualButton::Simultaneous(const VirtualButton& rhs) 
{
	if(this != &rhs) 
	{		
		return (this->m_fTimeStamp - rhs.m_fTimeStamp) == 0;
	} 
	else 
		return false; 
}

void VirtualButton::Unassign(KEYBOARD_KEYS eKey)
{
	std::list<KEYBOARD_KEYS>::iterator itr = m_lsAssignments.begin();
	for(;itr != m_lsAssignments.end(); itr++)
	{
		if(eKey == (*itr))
		{
			m_lsAssignments.erase(itr);
			break;
		}
	}
}