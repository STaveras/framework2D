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
		  Font* _font;
	std::string _text;

public:
	Text(void) : Widget(WIDGET_TYPE_TEXT), _font(nullptr), _text("") {}
	Text(Font* font, const std::string& text) : Widget(WIDGET_TYPE_TEXT), _font(font), _text(text) {}

	~Text(void) = default;

	Font* getFont(void) const { return _font; }
	void setFont(Font* font) { _font = font; }

	std::string getText(void) const { return _text; }
	void setText(const std::string& text) { _text = text; }
};

#endif  //_TEXT_H
