// Widget.h

#pragma once

#include "Renderable.h"

// This is a base class for UI widgets
class Widget : public Renderable
{
public:
	enum WIDGET_TYPE
	{
		WIDGET_TYPE_NULL,
		WIDGET_TYPE_BUTTON,
		WIDGET_TYPE_SLIDER,
		WIDGET_TYPE_SCROLLBAR,
		WIDGET_TYPE_TEXT,
		WIDGET_TYPE_IMAGE
	};

private:
	WIDGET_TYPE _widgetType = WIDGET_TYPE_NULL;

protected:
	Widget(void) : Renderable(RENDERABLE_TYPE_WIDGET) {}

	explicit Widget(WIDGET_TYPE widgetType) : Renderable(RENDERABLE_TYPE_WIDGET), _widgetType(widgetType) {}
};
