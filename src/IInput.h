// File: IInput.h
// Author: Stanley Taveras
// Created: 3/8/2010
// Modified: 3/8/2010

#pragma once

#include "Types.h"

#include "Keyboard.h"
#include "IMouse.h"

typedef class IInput
{
protected:
	Keyboard* _keyboard;
	Mouse* _mouse;

	// TODO: Add physical gamepad support

public:
	IInput(void) : _keyboard(NULL), _mouse(NULL) {}
	virtual ~IInput(void) = 0;

	virtual Keyboard* getKeyboard(void) { return _keyboard; }
	virtual Mouse* getMouse(void) { return _mouse; }

	virtual void initialize(void) = 0;
	virtual void update(void) = 0;
	virtual void shutdown(void) = 0;
}InputInterface;