// File: StateMachineEvent.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/17/2010

#if !defined(_STATEMACHINEEVENT_H)
#define _STATEMACHINEEVENT_H

#include "Event.h"
#include <string>

#define EVT_STATEMACHINE_EVENT "EVT_STATEMACHINE_EVENT"

class StateMachineEvent : public Event
{
	std::string m_szCondition;

public:
	StateMachineEvent(const char* szCondition, void* pSender):Event(EVT_STATEMACHINE_EVENT, pSender, Event::event_priority_normal), m_szCondition(szCondition){}

	bool operator==(const StateMachineEvent& rhs) { return (m_szCondition == rhs.m_szCondition); }
};

#endif  //_STATEMACHINEEVENT_H
