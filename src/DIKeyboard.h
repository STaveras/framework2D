
#pragma once

#include "IKeyboard.h"
#include "DirectInput.h"

class DIKeyboard : public IKeyboard, IDIDeviceComm
{
	friend class DirectInput;

   // This two buffers should probably just be in IKeyboard
	char m_cKeyBuffer[256];
	char m_cBackBuffer[256];

public:
	DIKeyboard(void);

	bool KeyDown(int nKey) { return (m_lpDevice && m_cKeyBuffer[nKey] & 0x80); }
	bool KeyUp(int nKey) { return (m_lpDevice && !(m_cKeyBuffer[nKey] & 0x80)); }
	bool KeyPressed(int nKey) { return (m_lpDevice && m_cBackBuffer[nKey] != m_cKeyBuffer[nKey] && KeyDown(nKey)); }
	bool KeyReleased(int nKey) { return (m_lpDevice && m_cBackBuffer[nKey] != m_cKeyBuffer[nKey] && KeyUp(nKey)); }
   
   bool Acquire(LPDIRECTINPUT8 pDI, HWND hWnd);
   void Update(void); // Think about adding a timer
};