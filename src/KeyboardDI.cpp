
#ifdef _WIN32

#include "KeyboardDI.h"

KeyboardDI::KeyboardDI(void)
{
   memset(m_cKeyBuffer, 0, _countof(m_cKeyBuffer));
}

bool KeyboardDI::acquire(LPDIRECTINPUT8 pDI, HWND hWnd)
{
   if (!FAILED(pDI->CreateDevice(GUID_SysKeyboard, &m_lpDevice, NULL))) {

      m_lpDevice->SetDataFormat(&c_dfDIKeyboard);
      m_lpDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

      return true;
   }
   return false;
}

void KeyboardDI::update(void)
{
   IDIDevice::update();

   if (m_lpDevice) {

      if (m_lpDevice->GetDeviceState(sizeof(m_cKeyBuffer), (LPVOID)&m_cKeyBuffer) == DIERR_INPUTLOST)
         m_bDeviceLost = true;

      _keys.beginFrame();
      for (int key = 0; key < 256; ++key) {
         _keys.set(key, (m_cKeyBuffer[key] & 0x80) != 0);
      }
   }
}

#endif