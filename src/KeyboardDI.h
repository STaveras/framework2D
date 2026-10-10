// KeyboardDI.h
// Keyboard input through DirectInput (the Win32/DirectX path).

#pragma once

#ifdef _WIN32

#include "DirectInput.h"
#include "Keyboard.h"

class KeyboardDI : public Keyboard, IDIDevice
{
	friend class DirectInput;

	// Raw DirectInput buffer, indexed by DIK scan code
	char m_cKeyBuffer[256];

public:
	KeyboardDI(void);

	void update(void) override;

	bool acquire(LPDIRECTINPUT8 pDI, HWND hWnd) override;
};

#endif
