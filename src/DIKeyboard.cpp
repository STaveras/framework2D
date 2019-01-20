
#include "DIKeyboard.h"

DIKeyboard::DIKeyboard(void)
{
   memset(m_cKeyBuffer, 0, _countof(m_cKeyBuffer));
}

bool DIKeyboard::Acquire(LPDIRECTINPUT8 pDI, HWND hWnd)
{
   if (!FAILED(pDI->CreateDevice(GUID_SysKeyboard, &m_lpDevice, NULL))) {

      m_lpDevice->SetDataFormat(&c_dfDIKeyboard);
      m_lpDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

      return true;
   }
   return false;
}

void DIKeyboard::Update(void)
{
   IDIDeviceComm::Update();

   if (m_lpDevice) {

      memcpy_s(m_cBackBuffer, 256, m_cKeyBuffer, 256);
      
      if (m_lpDevice->GetDeviceState(sizeof(m_cKeyBuffer), (LPVOID)&m_cKeyBuffer) == DIERR_INPUTLOST)
         m_bDeviceLost = true;
   }
}