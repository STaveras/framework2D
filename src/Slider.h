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
	float _increment;
	float _value;

public:
	void increment(void)
	{
		if (_value == 1.0f) {
			return; // Don't increment above 1
		}
		_value += _increment;
		if (_value > 1.0f) {
			_value = 1.0f;
		}
	}
	void decrement(void)
	{
		if (_value == 0.0f) {
			return; // Don't decrement below 0
		}
		_value -= _increment;
		if (_value < 0.0f) {
			_value = 0.0f;
		}
	}
};

#endif  //_SLIDER_H
