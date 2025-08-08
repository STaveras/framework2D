// File: InputEvent.h
// Author: Stanley Taveras
// Created: 3/15/2010
// Modified: 7/30/2025

#pragma once

#ifndef _INPUT_EVENT_H_
#define _INPUT_EVENT_H_

#include "Event.h"

#include <string>

#define EVT_KEYPRESSED "EVT_KEYPRESSED"
#define EVT_KEYRELEASED "EVT_KEYRELEASED"
#define EVT_KEYDOWN "EVT_KEYDOWN"
#define EVT_KEYUP "EVT_KEYUP"

class Controller;

class InputEvent : public Event
{
	float _timeStamp;
	Controller* _controller;
	std::string _actionName;

public:
	InputEvent(Event::event_key evtKey, void* pSender, float time, std::string actionName);

	float getTimeStamp(void) const { return _timeStamp; }
	Controller* getController(void) const { return _controller; }
	std::string getActionName(void) const { return _actionName; }
};

#endif // _INPUT_EVENT_H_