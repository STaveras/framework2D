// File: Button.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_BUTTON_H)
#define _BUTTON_H

#include "Widget.h"

class Square;

class Button : public Widget
{
	// Button's image changes to reflect the dimensions this area.
	Square* m_pArea;
};

#endif  //_BUTTON_H
