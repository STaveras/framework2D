// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/24/2010

#if !defined(_TYPES_H_)
#define _TYPES_H_

// TODO : Write your own wrapper for the DirectX math classes...

#ifdef _WIN32
#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>

#define vector2 D3DXVECTOR2 // TODO: As above, write the wrapper so you don't have to use raw d3d math calls in your code...
#define matrix4x4 D3DXMATRIX

#define rect RECT

#else

#include <glm/glm.hpp>

#define vector2 glm::vec2
#define matrix4x4 glm::mat4

#define strtok_s strtok_r
#define _strdup strdup
#define INFINITE INFINITY

struct rect {
    int left, top, right, bottom;
};

#endif

#define byte unsigned char
//#define color D3DCOLOR

#endif