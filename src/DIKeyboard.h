
#pragma once

#include "IKeyboard.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class DIKeyboard : public IKeyboard
{
	friend class DirectInput;

	bool m_bDeviceLost;
	LPDIRECTINPUTDEVICE8 m_lpDevice;
	char m_cKeyBuffer[256];
	char m_cBackBuffer[256];

public:
	DIKeyboard(void);
	~DIKeyboard(void);

	void Release(void);

	bool KeyDown(int nKey) { return (m_lpDevice && m_cKeyBuffer[nKey] & 0x80); }
	bool KeyUp(int nKey) { return (m_lpDevice && !(m_cKeyBuffer[nKey] & 0x80)); }
	bool KeyPressed(int nKey) { return (m_lpDevice && m_cBackBuffer[nKey] != m_cKeyBuffer[nKey] && KeyDown(nKey)); }
	bool KeyReleased(int nKey) { return (m_lpDevice && m_cBackBuffer[nKey] != m_cKeyBuffer[nKey] && KeyUp(nKey)); }

	void Update(void);
};