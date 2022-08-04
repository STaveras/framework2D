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

	virtual IKeyboard* GetKeyboard(void) { return _keyboard; }
	virtual IMouse* GetMouse(void) { return _mouse; }

	virtual void Initialize(void) = 0;
	virtual void Update(void) = 0;
	virtual void Shutdown(void) = 0;
}InputInterface;