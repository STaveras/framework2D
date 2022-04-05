// File: Input.h
#pragma once
#include "IInput.h"
#include "DirectInput.h"
namespace Input
{
#ifdef _WIN32
	static IInput* CreateDirectInputInterface(HWND hWnd, HINSTANCE hInstance)
	{
		DirectInput* pInstance = new DirectInput(hInstance, hWnd);
		return (IInput*)pInstance;
	}
#endif

	static void DestroyInputInterface(IInput* pInput) { if(pInput) delete pInput; }
}