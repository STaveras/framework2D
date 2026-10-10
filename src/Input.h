// File: Input.h
// Creates the desktop platform's input devices. The caller owns the result.
#pragma once
#include "IInput.h"
#include "DirectInput.h"
#include "PlatformInput.h"
#include "Window.h"

#include <memory>

namespace Input
{
#ifdef _WIN32
	inline std::unique_ptr<IInput> createDirectInputInterface(HWND hWnd, HINSTANCE hInstance)
	{
		return std::make_unique<DirectInput>(hInstance, hWnd);
	}
#endif
	inline std::unique_ptr<IInput> createInputInterface(Window* window)
	{
		return std::make_unique<PlatformInput>(window);
	}
}
