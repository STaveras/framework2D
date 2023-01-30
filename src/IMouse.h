// File: IMouse.h
// Author: Stanley Taveras
// Created: 3/4/2010
// Modified: 3/4/2010

// TODO: Debug lag when mouse moves?

#include "Types.h"

#if !defined(_IMOUSE_H_)
#define _IMOUSE_H_

#include "Positionable.h"

typedef class IMouse : public Positionable
{
public:
   virtual bool buttonPressed(MOUSE_BUTTONS eBtn) = 0;
   virtual bool buttonReleased(MOUSE_BUTTONS eBtn) = 0;
   virtual bool buttonDown(MOUSE_BUTTONS eBtn) = 0;
   virtual bool buttonUp(MOUSE_BUTTONS eBtn) = 0;
}Mouse;

#endif