// File: WidgetGroup.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_WIDGETGROUP_H)
#define _WIDGETGROUP_H

#include "Types.h"
#include <list>

class Widget;

class WidgetGroup
{
	vector2 m_Position;
	vector2 m_Dimensions;
	const char* m_szName;
	std::list<Widget*> m_lsWidgets;
};

#endif  //_WIDGETGROUP_H
