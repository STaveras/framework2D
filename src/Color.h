// color.h
#pragma once
#include "Types.h"
#define DEFAULT_KEY_COLOR 0xFFFF00FF
namespace framework {
	typedef union color
	{
		unsigned long _color;
		struct{	byte a;	byte r;	byte g;	byte b; };

		color(void):_color(0xFFFFFFFF){}
		color(unsigned int value){ _color = value; }

	#ifdef D3DCOLOR_DEFINED
		operator DWORD()
		{
			return _color;
		}
	#endif
		const color& operator=(unsigned long value) { _color = value; return *this; }
		const color& operator=(const color& value) { _color = value._color; return *this; }

	}Color;
}
using framework::Color;