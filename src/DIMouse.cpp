
#include "DIMouse.h"

#ifdef _DEBUG

#include "Debug.h"
#include "Timer.h"
#include <stdio.h>

#endif

bool DIMouse::Acquire(LPDIRECTINPUT8 pDI, HWND hWnd)
{
   if (!FAILED(pDI->CreateDevice(GUID_SysMouse, &m_lpDevice, NULL))) {

      m_lpDevice->SetDataFormat(&c_dfDIMouse2);
      m_lpDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE); //DISCL_EXCLUSIVE

      DIPROPDWORD dipdw;
      dipdw.diph.dwSize = sizeof(DIPROPDWORD);
      dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
      dipdw.diph.dwObj = 0;
      dipdw.diph.dwHow = DIPH_DEVICE;
      dipdw.dwData = 20;

      m_lpDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

      return true;
   }
   return false;
}

void DIMouse::update(void) {

   IDIDeviceComm::update();

   if (m_lpDevice) {
      memcpy_s(&_mouseStateOld, sizeof(DIMOUSESTATE2), &_mouseState, sizeof(DIMOUSESTATE2));

      if (SUCCEEDED(m_lpDevice->Poll())) {
         if (m_lpDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &_mouseState) == DI_OK) {
            this->SetPosition(vector2((float)_mouseState.lX, (float)_mouseState.lY));
         }
      }
      else
         m_bDeviceLost = true;
   }

#ifdef _DEBUG
   if (Debug::dbgMouse) {
      static Timer timer;
      timer.update();

      if (timer.getElapsedTime() > 1) {
         char buffer[128];
         sprintf_s(buffer, "Mouse pos(%i, %i)\nLbutton %s\n\n", _mouseState.lX, _mouseState.lY, (_mouseState.rgbButtons[0]) ? "true" : "false");
         OutputDebugString(buffer);
         timer.reset();
      }
   }
#endif
}