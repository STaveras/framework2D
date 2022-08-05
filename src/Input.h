// File: Input.h
#pragma once
#include "IInput.h"
#include "DirectInput.h"
#include "PlatformInput.h"
#include "Window.h"
namespace Input
{
#ifdef _WIN32
	static IInput* CreateDirectInputInterface(HWND hWnd, HINSTANCE hInstance)
	{
		return (IInput*)(new DirectInput(hInstance, hWnd));
	}
#endif
	static IInput* CreateInputInterface(Window* window)
	{
		return (IInput*)(new PlatformInput(window));
	}
	// This shouldn't be a thing
	static void DestroyInputInterface(IInput* pInput) {
      if (pInput) {
         delete pInput;
      }
   }
}