// File: IInput.h
// Author: Stanley Taveras
// Created: 3/8/2010
// Modified: 3/8/2010

#pragma once

#include "Types.h"

#include "InputEvent.h"

#include "Keyboard.h"
#include "IMouse.h"
#include "Gamepad.h"

class IInput
{
protected:
	IKeyboard* _keyboard;
	IMouse* _mouse;
	IGamepad* _gamepad;

public:
	IInput(void) : _keyboard(NULL), _mouse(NULL), _gamepad(NULL) {}
	virtual ~IInput(void) = 0;

	virtual IKeyboard* getKeyboard(void) { return _keyboard; }
	virtual IMouse* getMouse(void) { return _mouse; }
	virtual IGamepad* getGamepad(void) { return _gamepad; }

	virtual void initialize(void) = 0;
	virtual void update(void) = 0;
	virtual void shutdown(void) = 0;
};
