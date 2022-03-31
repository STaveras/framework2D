// color.h
#pragma once
#include "Types.h"
union color
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
	const color& operator=(unsigned long value) { _color = value; return *this;}
};