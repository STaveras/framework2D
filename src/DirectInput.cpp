
#include "DirectInput.h"

#include "DIKeyboard.h"
#include "DIMouse.h"

// TODO: Add support for gamepads

DirectInput::DirectInput(HINSTANCE hInstance, HWND hWnd) :
   m_hWnd(hWnd),
   m_lpDirectInput(NULL) {

   m_pKeyboard = new DIKeyboard();
   m_pMouse = new DIMouse();

   if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_lpDirectInput, NULL)))
      throw "Failed to create DirectInput COM interface";
}

DirectInput::~DirectInput(void) {
   delete m_pMouse;
   delete m_pKeyboard;
}

void DirectInput::Initialize(void)
{
   if (m_lpDirectInput)
   {
      // This stuff should be in DIKeyboard itself
      if (!FAILED(m_lpDirectInput->CreateDevice(GUID_SysKeyboard, &((DIKeyboard*)(m_pKeyboard))->m_lpDevice, NULL))) {

         ((DIKeyboard*)(m_pKeyboard))->m_lpDevice->SetDataFormat(&c_dfDIKeyboard);
         ((DIKeyboard*)(m_pKeyboard))->m_lpDevice->SetCooperativeLevel(m_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
      }
      else
         throw "Failed to create the keyboard";

      // ...Same as this, but again, keeping it "simple"
      if (!FAILED(m_lpDirectInput->CreateDevice(GUID_SysMouse, &((DIMouse*)m_pMouse)->m_lpDevice, NULL)))
      {
         ((DIMouse*)(m_pMouse))->m_lpDevice->SetDataFormat(&c_dfDIMouse);
         ((DIMouse*)(m_pMouse))->m_lpDevice->SetCooperativeLevel(m_hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
      }
      else
         throw "Failed to create the mouse";
   }
}

void DirectInput::Update(void)
{
   if (m_pKeyboard)
      ((DIKeyboard*)m_pKeyboard)->Update();

   if (m_pMouse)
      ((DIMouse*)m_pMouse)->Update();
}

void DirectInput::Shutdown(void)
{
   if (m_lpDirectInput)
   {
      if (m_pMouse) {
         ((DIMouse*)m_pMouse)->Release();
      }

      if (m_pKeyboard) {
         ((DIKeyboard*)m_pKeyboard)->Release();
      }

      m_lpDirectInput->Release();
      m_lpDirectInput = NULL;
   }
}