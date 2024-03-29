// File: InputEvent.h
// Author: Stanley Taveras
// Created: 3/15/2010
// Modified: 3/15/2010

#pragma once

#include "Event.h"
#include "Controller.h"

#define EVT_KEYPRESS   "EVT_KEYPRESSED"
#define EVT_KEYRELEASE "EVT_KEYRELEASED"

class InputEvent : public Event
{
	float _timeStamp;
	Controller* _controller;
	std::string _actionName;

public:
	InputEvent(Event::event_key evtKey, void* pSender, Controller* controller, float time, std::string actionName);

	float getTimeStamp(void) const { return _timeStamp; }
	Controller* getController(void) const { return _controller; }
	std::string getActionName(void) const { return _actionName; }
};