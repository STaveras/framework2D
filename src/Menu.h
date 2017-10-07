// File: Menu.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_MENU_H)
#define _MENU_H

#include "WidgetManager.h"

class Appearance;

class Menu
{
	Appearance* m_pAppearance;
	WidgetManager m_WidgetManager;

public:
	void Update(float fTime);
};

#endif  //_MENU_H
