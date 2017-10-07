// File: Widget.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_WIDGET_H)
#define _WIDGET_H

#include "Types.h"

class Appearance;
class Event;

class Widget
{
	bool m_bEnabled;
	Appearance* m_pApperance;
	vector2 m_Position;

public:
	virtual void OnClicked(Event* event) = 0;
	virtual void OnDoubleClicked(Event* event) = 0;
	virtual void OnHover(Event* event) = 0;
	virtual void OnKeyPress(Event* event) = 0;
};

#endif  //_WIDGET_H
