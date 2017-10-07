// File: InputEvent.h
// Author: Stanley Taveras
// Created: 3/15/2010
// Modified: 3/15/2010

#pragma once

#include "Event.h"
#include "VirtualGamePad.h"

class InputEvent : public Event
{
	float m_fTime;
	VirtualGamePad* m_pGamePad;
	ButtonID m_ButtonID;

public:
	InputEvent(Event::event_key evtKey, void* pSender, VirtualGamePad* pGamePad, float fTime, ButtonID btnID);

	float GetTimeStamp(void) const { return m_fTime; }
	VirtualGamePad* GetGamePad(void) const { return m_pGamePad; }
	ButtonID GetButtonID(void) const { return m_ButtonID; }
};