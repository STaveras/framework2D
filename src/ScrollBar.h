// File: ScrollBar.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_SCROLLBAR_H)
#define _SCROLLBAR_H

#include "Widget.h"
#include "SCROLLBAR_TYPE.h"
#include "Square.h"

class ScrollBar : public Widget
{
	SCROLLBAR_TYPE m_eType;
	float m_fScrollOffset;
	float m_fBarRatio = 1.0f;
	Square[3] m_CollisionBoxes;
};

#endif  //_SCROLLBAR_H
