// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/24/2010

#if !defined(_TYPES_H_)
#define _TYPES_H_

#define DEFAULT_DATA_PATH "./data/"

#ifndef _CUSTOM_STREAM_OBJECT
#include <fstream>

// This makes it easy to convert to our own file stream later
typedef std::fstream fpstream;
typedef std::ifstream ifpstream;
typedef std::ofstream ofpstream;
#else
// TODO: Use our own file stream for use with file packing
#endif

// TODO : Write your own wrapper for the DirectX math classes...

#ifdef _WIN32

#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <d3dx9math.h>

#else

#define GLFW_BUILD_UNIVERSAL
#define GLFW_USE_MENUBAR
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#endif

#if defined(GLM) || defined(__APPLE__) || defined(unix) || defined(__unix) || defined(__unix__)

#include <glm/glm.hpp>

#define vector2 glm::vec2
#define vector4 glm::vec4
#define matrix4x4 glm::mat4

#endif

#ifndef _WIN32

#define strtok_s strtok_r
#define _strdup strdup
#define INFINITE INFINITY

struct rect {
   int left, top, right, bottom;
};

#define RECT rect

#elif !defined(GLM)

// Windows
#define vector2 D3DXVECTOR2 // TODO: As above, write the wrapper so you don't have to use raw d3d math calls in your code...
#define matrix4x4 D3DXMATRIX
#define RECT RECT
// ---------------------------------------------------------------------------------------------------------------------

#endif

#define byte uint8_t

#include "color.h"

#include "KEYBOARD_KEYS.h"
#include "MOUSE_BUTTONS.h"

#define SAFE_DELETE(x) if(x) { delete x; x = NULL; }
#define COUNT_OF(arr) sizeof(arr) / sizeof(arr[0])

#endif