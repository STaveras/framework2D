// File: Text.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_TEXT_H)
#define _TEXT_H

#include "Widget.h"

class Font;

class Text : public Widget
{
	Font* m_pFont;
	const char* m_szText;
};

#endif  //_TEXT_H
