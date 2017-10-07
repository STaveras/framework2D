
#include "VirtualGamePad.h"

VirtualButton* VirtualGamePad::GetButtons(ButtonID eKey)
{
	std::list<VirtualButton>::iterator itr = m_lsButtons.begin();
	for(;itr != m_lsButtons.end(); itr++)
	{
		if(itr->GetButtonID() == eKey)
		{
			return &(*itr);
		}
	}

	return NULL;
}

void VirtualGamePad::RemoveButton(VirtualButton btn)
{
	std::list<VirtualButton>::iterator itr = m_lsButtons.begin();
	for(;itr != m_lsButtons.end(); itr++)
	{
		if(btn == (*itr))
		{
			m_lsButtons.erase(itr);
			break;
		}
	}
}

bool VirtualGamePad::ButtonPressed(ButtonID btnID)
{
	if(m_pInput)
	{
		std::list<VirtualButton>::iterator itr = m_lsButtons.begin();
		for(;itr != m_lsButtons.end(); itr++)
		{
			if(btnID == itr->GetButtonID())
			{
				std::list<KEYBOARD_KEYS>::const_iterator itr2 = itr->GetAssignments().begin();

				for(; itr2 != itr->GetAssignments().end(); itr2++)
				{
					if(m_pInput->GetKeyboard()->KeyPressed((*itr2)))
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

bool VirtualGamePad::ButtonReleased(ButtonID btnID)
{
	if(m_pInput)
	{
		std::list<VirtualButton>::iterator itr = m_lsButtons.begin();
		for(;itr != m_lsButtons.end(); itr++)
		{
			if(btnID == itr->GetButtonID())
			{
				std::list<KEYBOARD_KEYS>::const_iterator itr2 = itr->GetAssignments().begin();

				for(; itr2 != itr->GetAssignments().end(); itr2++)
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