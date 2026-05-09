
#include "DIMouse.h"

#ifdef _WIN32

#ifdef _DEBUG

#include "Debug.h"
#include "Timer.h"
#include <stdio.h>

#endif

bool DIMouse::_cursorIsInsideClient(void) const
{
   if (!_hWnd) {
      return false;
   }

   POINT screenPoint{};
   if (!GetCursorPos(&screenPoint)) {
      return false;
   }

   RECT clientRect{};
   if (!GetClientRect(_hWnd, &clientRect)) {
      return false;
   }

   POINT clientOrigin{ clientRect.left, clientRect.top };
   POINT clientMax{ clientRect.right, clientRect.bottom };
   if (!ClientToScreen(_hWnd, &clientOrigin) || !ClientToScreen(_hWnd, &clientMax)) {
      return false;
   }

   RECT screenClientRect{};
   screenClientRect.left = clientOrigin.x;
   screenClientRect.top = clientOrigin.y;
   screenClientRect.right = clientMax.x;
   screenClientRect.bottom = clientMax.y;
   return PtInRect(&screenClientRect, screenPoint) == TRUE;
}

bool DIMouse::_syncPositionToClientCursor(void)
{
   if (!_hWnd) {
      return false;
   }

   POINT screenPoint{};
   if (!GetCursorPos(&screenPoint) || !ScreenToClient(_hWnd, &screenPoint)) {
      return false;
   }

   this->setPosition(vector2((float)screenPoint.x, (float)screenPoint.y));
   return true;
}

bool DIMouse::acquire(LPDIRECTINPUT8 pDI, HWND hWnd)
{
   if (!FAILED(pDI->CreateDevice(GUID_SysMouse, &m_lpDevice, NULL))) {
      _hWnd = hWnd;

      m_lpDevice->SetDataFormat(&c_dfDIMouse2);
      m_lpDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE); //DISCL_EXCLUSIVE

      DIPROPDWORD dipdw;
      dipdw.diph.dwSize = sizeof(DIPROPDWORD);
      dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
      dipdw.diph.dwObj = 0;
      dipdw.diph.dwHow = DIPH_DEVICE;
      dipdw.dwData = 20;

      m_lpDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);
      _wasCursorInsideClient = _cursorIsInsideClient();
      if (!_syncPositionToClientCursor() && _hWnd) {
         RECT clientRect{};
         if (GetClientRect(_hWnd, &clientRect)) {
            this->setPosition(vector2(
               (float)(clientRect.right - clientRect.left) * 0.5f,
               (float)(clientRect.bottom - clientRect.top) * 0.5f));
         }
      }

      return true;
   }
   return false;
}

void DIMouse::update(void) {
   const bool wasDeviceLost = m_bDeviceLost;

   IDIDevice::update();

   if (m_lpDevice) {
      memcpy_s(&_mouseStateOld, sizeof(DIMOUSESTATE2), &_mouseState, sizeof(DIMOUSESTATE2));

      if (SUCCEEDED(m_lpDevice->Poll())) {
         if (m_lpDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &_mouseState) == DI_OK) {
            const bool cursorInsideClient = _cursorIsInsideClient();

            if ((!_wasCursorInsideClient && cursorInsideClient) ||
               (wasDeviceLost && !m_bDeviceLost && cursorInsideClient)) {
               _syncPositionToClientCursor();
            }
            else {
               vector2 position = this->getPosition() + vector2((float)_mouseState.lX, (float)_mouseState.lY);

               if (_hWnd) {
                  RECT clientRect{};
                  if (GetClientRect(_hWnd, &clientRect)) {
                     const float maxX = (float)(clientRect.right - clientRect.left);
                     const float maxY = (float)(clientRect.bottom - clientRect.top);

                     if (position.x < 0.0f) {
                        position.x = 0.0f;
                     }
                     else if (position.x > maxX) {
                        position.x = maxX;
                     }

                     if (position.y < 0.0f) {
                        position.y = 0.0f;
                     }
                     else if (position.y > maxY) {
                        position.y = maxY;
                     }
                  }
               }

               this->setPosition(position);
            }

            _wasCursorInsideClient = cursorInsideClient;
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
         sprintf_s(buffer, sizeof(buffer), "Mouse pos(%i, %i)\nLbutton %s\n\n", _mouseState.lX, _mouseState.lY, (_mouseState.rgbButtons[0]) ? "true" : "false");
         DEBUG_MSG(buffer);
         timer.reset();
      }
   }
#endif
}

#endif
