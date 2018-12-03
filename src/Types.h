// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/24/2010

#if !defined(_TYPES_H_)
#define _TYPES_H_

// TODO : Write your own wrapper for the DirectX math classes...

#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>

#define byte unsigned char
//#define color D3DCOLOR
#define vector2 D3DXVECTOR2 // TODO: As above, write the wrapper so you don't have to use raw d3d math calls in your code...
#define matrix4x4 D3DXMATRIX
#define rect RECT

#include "KEYBOARD_KEYS.h"
#include "MOUSE_BUTTONS.h"

#endif