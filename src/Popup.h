// File: Popup.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_POPUP_H)
#define _POPUP_H

#include "Widget.h"

#include <list>

class Popup : public Widget
{
	std::list<Widget*> m_lsWidgets;

public:
	void OnOpen();
	bool OnClose();
};

#endif  //_POPUP_H
