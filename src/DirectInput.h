// File: DirectInput.h
// Author: Stanley Taveras
// Created: 3/7/2010
// Modified: 3/7/2010

#pragma once

#ifdef _WIN32

#include "IInput.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "DIKeyboard.h"

class DirectInput : public IInput
{
	HWND m_hWnd;
	LPDIRECTINPUT8 m_lpDirectInput;

public:
	DirectInput(void);
	DirectInput(HINSTANCE hInstance, HWND hWnd);
	~DirectInput(void){}

	void Initialize(void);
	void Update(void);
	void Shutdown(void);
};

#endif