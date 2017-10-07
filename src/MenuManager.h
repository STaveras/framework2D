// File: MenuManager.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_MENUMANAGER_H)
#define _MENUMANAGER_H

#include "Factory.h"
#include "Menu.h"

class MenuManager
{
	Factory<Menu> m_Menus;
};

#endif  //_MENUMANAGER_H
