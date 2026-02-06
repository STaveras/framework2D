// color.h
#pragma once

#include "Types.h"

namespace framework {
	
	typedef union color
	{
		unsigned long _color;

//#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__)
		struct { byte a; byte r; byte g; byte b; };
//#else
		//struct { byte b; byte g; byte r; byte a; };
//#endif

		color(void):_color(0xFFFFFFFF){}
		color(unsigned long value){ _color = value; }
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
		const color& operator=(unsigned long value) { _color = value; return *this; }
		const color& operator=(const color& value) { _color = value._color; return *this; }

	}Color;
}
using framework::Color;
