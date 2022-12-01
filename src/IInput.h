// File: IInput.h
// Author: Stanley Taveras
// Created: 3/8/2010
// Modified: 3/8/2010

#pragma once

#include "Types.h"

#include "IKeyboard.h"
#include "IMouse.h"

typedef class IInput
{
protected:
	IKeyboard* _keyboard;
	IMouse* _mouse;

	// TODO: Add physical gamepad support

public:
	IInput(void) : _keyboard(NULL), _mouse(NULL) {}
	virtual ~IInput(void) = 0;

	virtual IKeyboard* getKeyboard(void) { return _keyboard; }
	virtual IMouse* getMouse(void) { return _mouse; }

	virtual void initialize(void) = 0;
	virtual void update(void) = 0;
	virtual void shutdown(void) = 0;
}InputInterface;