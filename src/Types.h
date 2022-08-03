// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 5/24/2022

#if !defined(_TYPES_H_)
#define _TYPES_H_

// TODO : Write your own wrapper for the DirectX math classes; or use GLM directly and just transpose matrices if necessary

#ifdef _WIN32

#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <d3dx9math.h>

#else

#define byte unsigned char
#define strtok_s strtok_r
#define _strdup strdup
#define INFINITE INFINITY

struct rect {
   int left, top, right, bottom;
};

#define RECT rect

#endif

#if defined(GLM) || defined(__APPLE__) || defined(__linux__) || defined(__unix__)

#include <glm/glm.hpp>

struct vector2 : public glm::vec2 {
   operator D3DXVECTOR2() const { return D3DXVECTOR2(this->x, this->y); }
};

#define vector2 glm::vec2
#define matrix4x4 glm::mat4

#else

#define vector2 D3DXVECTOR2 
#define matrix4x4 D3DXMATRIX
#define RECT RECT

// TODO: Remove any direct references to D3DXVECTOR2 and related functions

#endif

#include "color.h"

#include "KEYBOARD_KEYS.h"
#include "MOUSE_BUTTONS.h"

#endif