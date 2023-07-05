// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 5/24/2022

#if !defined(_TYPES_H_)
#define _TYPES_H_

// Version should at some point be managed by some build management/CI system (e.g. Jenkins, Travis, etc.)
#define FRAMEWORK_VERSION "0.05"

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

#pragma warning(push)
#pragma warning(disable: 26817)

#include <simdjson.h>
#include <tinyxml2.h>

#pragma warning(pop)

#pragma comment(lib, "simdjson.lib")
#pragma comment(lib, "tinyxml2.lib")

#define SAFE_DELETE(x) if(x) { delete x; x = NULL; }
#define COUNT_OF(arr) sizeof(arr) / sizeof(arr[0])

#define byte uint8_t

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define sleep Sleep

#include <windows.h>
#include <shellapi.h>

// Suppressing warnings from DirectX headers
#pragma warning(push)
#pragma warning(disable: 26495)
#pragma warning(disable: 28251)

#define DIRECTINPUT_VERSION 0x0800

#include <direct.h>
#include <dinput.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <d3dx9math.h>

#pragma warning(pop)

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

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

#if defined(GLM) || defined(__APPLE__) || defined(__linux__) || defined(__unix__)

#include <glm/glm.hpp>

#endif

#include <GLFW/glfw3.h>

#ifdef _WIN32
#pragma comment(lib, "glfw3.lib")
#endif

#include "Maths.h"
#include "Color.h"

#include "KEYBOARD_KEYS.h"
#include "MOUSE_BUTTONS.h"

#endif