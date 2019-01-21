// File: Input.h
#pragma once
#include "DirectInput.h"
namespace Input
{
	static IInput* CreateDirectInputInterface(HWND hWnd, HINSTANCE hInstance)
	{
		return (IInput*)(new DirectInput(hInstance, hWnd));
	}

	static void DestroyInputInterface(IInput* pInput) {
      if (pInput) {
         delete pInput;
      }
   }
}