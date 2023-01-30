// File: DirectInput.h
// Author: Stanley Taveras
// Created: 3/7/2010
// Modified: 3/7/2010

#pragma once

#ifdef _WIN32

#include "IInput.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "Timer.h"

// Interface to handle common direct input device tasks
class IDIDevice
{
protected:
   bool m_bDeviceLost = true;
   LPDIRECTINPUTDEVICE8 m_lpDevice;

public:

   virtual bool acquire(LPDIRECTINPUT8 pDI, HWND hWnd = NULL) = 0;

   virtual void release(void)
   {
      if (m_lpDevice)
      {
         if (!m_bDeviceLost) {
            m_lpDevice->Unacquire();
         }

         m_lpDevice->Release();
         m_lpDevice = NULL;
      }
   }

   virtual void update(void)
   {
      if (m_bDeviceLost) {
         if (m_lpDevice) {
            if (!FAILED(m_lpDevice->Acquire())) {
               m_bDeviceLost = false;
            }
         }
      }
   }
};

class DirectInput : public IInput
{
   HWND m_hWnd;
   LPDIRECTINPUT8 m_lpDirectInput;

public:
   DirectInput(HINSTANCE hInstance, HWND hWnd);
   ~DirectInput(void);

	void initialize(void);
	void update(void);
	void shutdown(void);
};

#endif
