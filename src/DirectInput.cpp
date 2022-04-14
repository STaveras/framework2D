
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
	   if (m_pKeyboard && !((DIKeyboard*)m_pKeyboard)->Acquire(m_lpDirectInput, m_hWnd))
         throw "Failed to create the keyboard";

      if (m_pMouse && !((DIMouse*)m_pMouse)->Acquire(m_lpDirectInput, m_hWnd))
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