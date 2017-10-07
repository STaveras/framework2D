// File: Slider.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_SLIDER_H)
#define _SLIDER_H

#include "Widget.h"

class Square;

class Slider : public Widget
{
	Square* m_pSlider;
	float m_fIncrement;
	float m_fValue;

public:
	void Increment();
	void Decrement();
};

#endif  //_SLIDER_H
