// color.h
#pragma once

#include "Types.h"
#include <cstdint>

namespace framework {
	
	typedef union color
	{
		uint32_t _color;

		// Color storage is canonicalized as 0xAARRGGBB across APIs.
		// On little-endian CPUs, this byte layout keeps r/g/b/a channel
		// access consistent with the packed integer value.
		struct { byte b; byte g; byte r; byte a; };

		color(void):_color(0xFFFFFFFF){}
		color(unsigned long value){ _color = static_cast<uint32_t>(value); }
		color(float alpha, float red, float green, float blue) { 
			a = (int)(alpha * 255);
			r = (int)(red * 255);
			g = (int)(green * 255);
			b = (int)(blue * 255); 
		}
		color(const color& other) = default;

	//#ifdef D3DCOLOR_DEFINED
		//operator DWORD()
		//{
		//	if (DIRECT3D_VERSION != NULL) {
		//		// Swap R and B for DirectX (BGRA)
		//		return ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | ((DWORD)b);
		//	}

		//	return _color;
		//}
	//#elif defined (__APPLE__)
	//	operator MTLClearColor()
	//	{
	//		// We might need to reverse endian-ness here, depending on how the color is stored
	//		return MTLClearColorMake(r/255.0, g/255.0, b/255.0, a/255.0);
	//	}
	//#endif
		const color& operator=(unsigned long value) { _color = static_cast<uint32_t>(value); return *this; }
		const color& operator=(const color& value) { _color = value._color; return *this; }

	}Color;
}
using framework::Color;
