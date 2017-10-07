// File: VirtualButton.h
// Author: Stanley Taveras
// Created: 3/17/2010
// Modified: 3/17/2010

#pragma once

#include "KEYBOARD_KEYS.h"
#include <list>
#include <string>

typedef std::string ButtonID;

class VirtualButton
{
    float m_fTimeStamp;
    ButtonID m_ButtonID;
    std::list<KEYBOARD_KEYS> m_lsAssignments; // TODO: This should support any device (like arcade sticks...)

public:
	VirtualButton(void):m_fTimeStamp(0),m_ButtonID(""){}
	VirtualButton(ButtonID btnID):m_fTimeStamp(0),m_ButtonID(btnID){}
	VirtualButton(ButtonID btnID, KEYBOARD_KEYS key):m_fTimeStamp(0),m_ButtonID(btnID){m_lsAssignments.push_back(key);}

	float GetTimeStamp(void) const { return m_fTimeStamp; }
	ButtonID GetButtonID(void) const { return m_ButtonID; }
	std::list<KEYBOARD_KEYS>& GetAssignments(void) { return m_lsAssignments; }

	void SetTimeStamp(float fTime) { m_fTimeStamp = fTime; }
	void SetButtonID(ButtonID btnID) { m_ButtonID = btnID; }

	void Assign(KEYBOARD_KEYS eKey) { m_lsAssignments.push_back(eKey); }
	void Unassign(KEYBOARD_KEYS eKey);

	bool Before(const VirtualButton& rhs);
	bool After(const VirtualButton& rhs);
	bool Simultaneous(const VirtualButton& rhs);

	bool operator==(const VirtualButton& rhs) { return (m_ButtonID == rhs.m_ButtonID); }
};