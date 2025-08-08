// File: Input.h
#pragma once
#include "IInput.h"
#include "DirectInput.h"
#include "PlatformInput.h"
#include "Window.h"
namespace Input
{
#ifdef _WIN32
	static IInput* createDirectInputInterface(HWND hWnd, HINSTANCE hInstance)
	{
		return (IInput*)(new DirectInput(hInstance, hWnd));
	}
#endif
	static IInput* createInputInterface(Window* window)
	{
		return (IInput*)(new PlatformInput(window));
	}

	// This shouldn't be a thing
	static void destroyInputInterface(IInput* pInput) 
	{
      if (pInput) {
         delete pInput;
      }
   }
}