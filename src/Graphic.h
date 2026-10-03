// File: Graphic.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_GRAPHIC_H)
#define _GRAPHIC_H

#include "Widget.h"
#include "Sprite.h"

// Absolutely no use other than to display a simple graphic...
class Graphic : public Widget, Sprite
{
public:
	Graphic(void) : Widget(), Sprite() {

	}
};

#endif  //_GRAPHIC_H
