// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 5/24/2022

#if !defined(_TYPES_H_)
#define _TYPES_H_

// Version should at some point be managed by some build management/CI system (e.g. Jenkins, Travis, etc.)
#define FRAMEWORK_VERSION "0.02"

#define DEFAULT_DATA_PATH "./data/"

// TODO: Query the device for the bare minimum usable resolution
#define GLOBAL_WIDTH  640
#define GLOBAL_HEIGHT 480

#ifndef _CUSTOM_STREAM_OBJECT
#include <fstream>

// This makes it easy to convert to our own file stream later
typedef std::fstream fpstream;
typedef std::ifstream ifpstream;
typedef std::ofstream ofpstream;
#else
// TODO: Use our own file stream for use with file packing
#endif

#define SAFE_DELETE(x) if(x) { delete x; x = NULL; }
#define COUNT_OF(arr) sizeof(arr) / sizeof(arr[0])

#define byte uint8_t

#ifdef _WIN32

#define NOMINMAX

#include <windows.h>
#include <shellapi.h>

#include <direct.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <d3dx9math.h>

#include <vulkan/vulkan.h>

#define RECT RECT

#define getcwd _getcwd

#else

#if __APPLE__
#include <MoltenVK/mvk_vulkan.h>
#include <MoltenVK/vk_mvk_moltenvk.h>
#else
#define GLFW_INCLUDE_VULKAN
#endif

#define GLFW_BUILD_UNIVERSAL
#define GLFW_USE_MENUBAR

#define localtime_s(n, ts) localtime_r(ts, n)

#define sprintf_s printf
#define strtok_s strtok_r
#define _strdup strdup
#define INFINITE INFINITY
#define UINT unsigned int

struct rect {
   int left, top, right, bottom;
};

#define RECT rect

#endif

#include <GLFW/glfw3.h>

#if defined(GLM) || defined(__APPLE__) || defined(__linux__) || defined(__unix__)

#include <glm/glm.hpp>

#ifdef _WIN32
struct vector2 : public glm::vec2 {
   operator D3DXVECTOR2() const { return D3DXVECTOR2(this->x, this->y); }
};
#else
#include "Maths.h"
#endif

#else

// TODO: Remove any direct references to D3DXVECTOR2 and related functions
// TODO: Write your own wrapper for the DirectX math classes...

// Windows
#if !defined(GLM) // We're going to have to check this at run time if we want to swap between DX and VK on the fly
#define vector2 D3DXVECTOR2 // TODO: As above, write the wrapper so you don't have to use raw d3d math calls in your code...
#define vector3 D3DXVECTOR3
#define vector4 D3DXVECTOR4
#define matrix4x4 D3DXMATRIX // i leik trucks :]
#endif
// ---------------------------------------------------------------------------------------------------------------------

#endif

#include "Color.h"

#include "KEYBOARD_KEYS.h"
#include "MOUSE_BUTTONS.h"

#endif