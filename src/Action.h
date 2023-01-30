// File: Action.h
// Author: Stanley Taveras
// Created: 3/17/2010
// Modified: 3/17/2010

#pragma once

#include "Types.h"

#include "Keyboard.h"

#include <list>
#include <string>

// Change "KeyBoardKey" with "InputEvent" to support any Input device (keyboard, gamepad, mouse, etc.)

class Action
{
    float _actionTime;
    std::string _actionName;
    std::list<Keyboard::KEY> _inputAssignments; // TODO: This should support any device (like arcade sticks...)

public:
	Action(void):_actionTime(0),_actionName(""){}
	Action(std::string actionName):_actionTime(0),_actionName(actionName){}
	Action(std::string actionName, Keyboard::KEY key):_actionTime(0),_actionName(actionName){_inputAssignments.push_back(key);}

			float getActionTime(void) const { return _actionTime; }
	std::string getActionName(void) const { return _actionName; }

	void setActionTime(float fTime) { _actionTime = fTime; }
	void setActionName(std::string actionName) { _actionName = actionName; }

	std::list<Keyboard::KEY>& getAssignments(void) { return _inputAssignments; }

	// use 'inputIdentifier' to make a platform independent hash of an input method event 
	void assign(Keyboard::KEY eKey) { _inputAssignments.push_back(eKey); }
	void unassign(Keyboard::KEY eKey);

	bool before(const Action& rhs);
	bool after(const Action& rhs);
	bool simultaneous(const Action& rhs);

	bool operator==(const Action& rhs) { return (_actionName == rhs._actionName); }
};
