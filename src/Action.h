// File: Action.h
// Author: Stanley Taveras
// Created: 3/17/2010
// Modified: 3/17/2010

#pragma once

#include "Types.h"

#include "Keyboard.h"
#include "Gamepad.h"

#include <list>
#include <string>

// A named action can combine assignments from multiple input devices.

class Action
{
public:
	struct AxisAssignment
	{
		Gamepad::Axis axis;
		float threshold;

		bool operator==(const AxisAssignment& rhs) const
		{
			return axis == rhs.axis && threshold == rhs.threshold;
		}
	};

private:
	bool _active = false; // Whether the action is currently active or not
    float _actionTime;
    std::string _actionName;
    std::list<Keyboard::KEY> _inputAssignments;
	std::list<Gamepad::Button> _gamepadButtonAssignments;
	std::list<AxisAssignment> _gamepadAxisAssignments;

public:
	Action(void):_actionTime(0),_actionName(""){}
	Action(std::string actionName):_actionTime(0),_actionName(actionName){}
	Action(std::string actionName, Keyboard::KEY key):_actionTime(0),_actionName(actionName){_inputAssignments.push_back(key);}
	Action(std::string actionName, Gamepad::Button button):_actionTime(0),_actionName(actionName){_gamepadButtonAssignments.push_back(button);}

	bool isActive(void) const { return _active; }
	void setActive(bool active) { _active = active; }

	float getActionTime(void) const { return _actionTime; }
	void setActionTime(float fTime) { _actionTime = fTime; }

	std::string getActionName(void) const { return _actionName; }
	void setActionName(std::string actionName) { _actionName = actionName; }

	std::list<Keyboard::KEY>& getAssignments(void) { return _inputAssignments; }
	const std::list<Keyboard::KEY>& getAssignments(void) const { return _inputAssignments; }
	std::list<Gamepad::Button>& getGamepadButtonAssignments(void) { return _gamepadButtonAssignments; }
	const std::list<Gamepad::Button>& getGamepadButtonAssignments(void) const { return _gamepadButtonAssignments; }
	std::list<AxisAssignment>& getGamepadAxisAssignments(void) { return _gamepadAxisAssignments; }
	const std::list<AxisAssignment>& getGamepadAxisAssignments(void) const { return _gamepadAxisAssignments; }

	void assign(Keyboard::KEY eKey) { _inputAssignments.push_back(eKey); }
	void assign(Gamepad::Button button) { _gamepadButtonAssignments.push_back(button); }
	// Positive thresholds activate above the threshold; negative thresholds below it.
	void assignAxis(Gamepad::Axis axis, float threshold) { _gamepadAxisAssignments.push_back({ axis, threshold }); }
	void unassign(Keyboard::KEY eKey);
	void unassign(Gamepad::Button button);
	void unassignAxis(Gamepad::Axis axis, float threshold);

	bool before(const Action& rhs);
	bool after(const Action& rhs);
	bool simultaneous(const Action& rhs);

	bool operator==(const Action& rhs) { return (_actionName == rhs._actionName); }
};
