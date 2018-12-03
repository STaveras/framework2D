// File: IInput.h
// Author: Stanley Taveras
// Created: 3/8/2010
// Modified: 3/8/2010

#pragma once

#include "Types.h"

#include "IKeyboard.h"
#include "IMouse.h"

class IInput
{
protected:
	IKeyboard* m_pKeyboard;
	IMouse* m_pMouse;

	// TODO: Add physical gamepad support

public:
	IInput(void):m_pKeyboard(NULL),m_pMouse(NULL){}
   virtual ~IInput(void) = 0 {}

	virtual IKeyboard* GetKeyboard(void) { return m_pKeyboard; }
	virtual IMouse* GetMouse(void) { return m_pMouse; }

	virtual void Initialize(void) = 0;
	virtual void Update(void) = 0;
	virtual void Shutdown(void) = 0;
};