// File: DirectInput.h
// Author: Stanley Taveras
// Created: 3/7/2010
// Modified: 3/7/2010

#pragma once

#include "IInput.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// Interface to handle common direct input device tasks
class IDIDeviceComm
{
protected:
   bool m_bDeviceLost = true;
   LPDIRECTINPUTDEVICE8 m_lpDevice;

public:
   virtual void Release(void)
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

   virtual void Update(void)
   {
      if (m_bDeviceLost) {
         if (m_lpDevice) {
            if (!FAILED(m_lpDevice->Acquire()))
               m_bDeviceLost = false;
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

   void Initialize(void);
   void Update(void);
   void Shutdown(void);
};