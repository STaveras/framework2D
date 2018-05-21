#pragma once

enum Platform
{
   Windows = 0x01
};

#ifdef WIN32

#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>

//#define color D3DCOLOR
#define vector2 D3DXVECTOR2 // TODO: As above, write the wrapper so you don't have to use raw d3d math calls in your code...
#define matrix4x4 D3DXMATRIX
#define rect RECT

#endif