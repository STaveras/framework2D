// File: IMouse.h
// Author: Stanley Taveras
// Created: 3/4/2010
// Modified: 3/4/2010

#if !defined(_IMOUSE_H_)
#define _IMOUSE_H_

#include "Types.h"

class IMouse
{
	vector2 m_Position;

public:
	vector2 GetPosition(void) const { return m_Position; }
	void SetPosition(vector2 position) { m_Position = position; }

   virtual bool buttonPressed(MOUSE_BUTTONS eBtn) = 0;
   virtual bool buttonReleased(MOUSE_BUTTONS eBtn) = 0;
   virtual bool ButtonDown(MOUSE_BUTTONS eBtn) = 0;
   virtual bool ButtonUp(MOUSE_BUTTONS eBtn) = 0;
};

#endif