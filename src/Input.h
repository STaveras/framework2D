// File: Input.h
#pragma once
#include "DirectInput.h"
namespace Input
{
	static IInput* CreateDirectInputInterface(HWND hWnd, HINSTANCE hInstance)
	{
		DirectInput* pInstance = new DirectInput(hInstance, hWnd);
		return (IInput*)pInstance;
	}

	static void DestroyInputInterface(IInput* pInput) { if(pInput) delete pInput; }
}