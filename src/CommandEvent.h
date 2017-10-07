
#pragma once

#include "Event.h"

class Command;

class CommandEvent : public Event
{
	Command* m_pCommand;

public:
	CommandEvent(Command* pCommand, void* pSender):Event("EVT_COMMAND", pSender, Event::event_priority_normal), m_pCommand(pCommand){}

	Command* GetCommand(void) { return m_pCommand; }
};