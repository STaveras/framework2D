// File: Types.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/08/2026

#if !defined(_TYPES_H_)
#define _TYPES_H_

#include <cstdint>

// Version should at some point be managed by some build management/CI system (e.g. Jenkins, Travis, etc.)
#define FRAMEWORK_VERSION "0.10"

#define DEFAULT_DATA_PATH "./data/"

// TODO: Query the device for the bare minimum usable resolution
#define GLOBAL_WIDTH  640
#define GLOBAL_HEIGHT 480

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26817)
#pragma warning(disable: 26437)
#endif

#define SAFE_DELETE(x) if(x) { delete x; x = NULL; }
#define COUNT_OF(arr) sizeof(arr) / sizeof(arr[0])

using byte = uint8_t;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#define sleep Sleep

#include <windows.h>
#include <shellapi.h>

// Suppressing warnings from DirectX headers
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 26495)
#pragma warning(disable: 28251)
#endif

#define DIRECTINPUT_VERSION 0x0800

#include <direct.h>
#include <dinput.h>

#include <d3d9.h>
#include <d3dx9.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

#define getcwd _getcwd

#else

#if __APPLE__

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

// Prevent GLFW from including legacy OpenGL headers on macOS
#define GLFW_INCLUDE_NONE

// #define GLM
#define GLFW_EXPOSE_NATIVE_COCOA

#include <Metal/Metal.h>

// #include <Metal/Metal.hpp>

#include <MoltenVK/mvk_vulkan.h>
#include <MoltenVK/vk_mvk_moltenvk.h>

#else
#define GLFW_INCLUDE_VULKAN
#endif

#define GLFW_BUILD_UNIVERSAL
#define GLFW_USE_MENUBAR

#define localtime_s(n, ts) localtime_r(ts, n)

#define ZeroMemory(p, sz) memset((p), 0, (sz))

//#define strncpy_s strncpy
#include <cstdio>
#define sprintf_s snprintf
#define strtok_s strtok_r
#define _strdup strdup
#define INFINITE INFINITY
#define UINT unsigned int

typedef struct rect {
   int left, top, right, bottom;
}RECT;

#define DEFAULT_KEY_COLOR 0xFFFF00FF

#endif

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#endif

#ifndef _DEBUG
#ifndef DEBUG_MSG
#define DEBUG_MSG(msg) // do nothing
#endif
#endif

#include "Maths.h"
#include "Color.h"

#include <GLFW/glfw3.h>

#include <simdjson.h>
#include <tinyxml2.h>

//union rekt {
//   struct {
//      int32_t left, top, right, bottom;
//   };
//   RECT _rect;
//#ifdef VULKAN_H_
//   VkRect2D _rect2D;
//#endif
//};

#ifdef _WIN32
#pragma warning(pop)
#if defined(_MSC_VER)
#pragma comment(lib, "simdjson.lib")
#pragma comment(lib, "tinyxml2.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3.lib")
#endif
#else
#include <GLFW/glfw3native.h>
#endif

#include "KEYBOARD_KEYS.h"
#include "MOUSE_BUTTONS.h"

#include "FileSystem.h"

typedef FileSystem::File::Stream FileStream;

#endif
