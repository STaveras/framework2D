// File: WidgetManager.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_WIDGETMANAGER_H)
#define _WIDGETMANAGER_H

#include "Factory.h"
#include "Widget.h"
#include "WidgetGroup.h"
#include <map>

class WidgetManager
{
	std::map<WidgetGroup, Factory<Widget>> m_Widgets;
	Factory<WidgetGroup> m_WidgetGroups;
};

#endif  //_WIDGETMANAGER_H
