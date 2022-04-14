
#include "DirectInput.h"

#include "DIKeyboard.h"
#include "DIMouse.h"

// TODO: Add support for gamepads

DirectInput::DirectInput(HINSTANCE hInstance, HWND hWnd) :
   m_hWnd(hWnd),
   m_lpDirectInput(NULL) {

   _keyboard = new DIKeyboard();
   _mouse = new DIMouse();

   if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_lpDirectInput, NULL)))
      throw "Failed to create DirectInput COM interface";
}

DirectInput::~DirectInput(void) {
   
   SAFE_DELETE(_keyboard);
   SAFE_DELETE(_mouse);
   
   // if (m_lpDirectInput) {
   //    if (_mouse) {
   //       ((DIMouse*)_mouse)->Release();
   //    }

   //    if (_keyboard) {
   //       ((DIKeyboard*)_keyboard)->Release();
   //    }

   //    m_lpDirectInput->Release();
   //    m_lpDirectInput = NULL;
   // }
}

void DirectInput::Initialize(void)
{
   // We should honestly just do all this stuff in the constructor
   if (m_lpDirectInput)
   {
	   if (_keyboard && !((DIKeyboard*)_keyboard)->Acquire(m_lpDirectInput, m_hWnd))
         throw "Failed to create the keyboard";

      if (_mouse && !((DIMouse*)_mouse)->Acquire(m_lpDirectInput, m_hWnd))
         throw "Failed to create the mouse";
   }
}

void DirectInput::Update(void)
{
   if (_keyboard)
      ((DIKeyboard*)_keyboard)->Update();

   if (_mouse)
      ((DIMouse*)_mouse)->Update();
}

void DirectInput::Shutdown(void)
{
   if (m_lpDirectInput)
   {
      if (_mouse) {
         ((DIMouse*)_mouse)->Release();
      }

      if (_keyboard) {
         ((DIKeyboard*)_keyboard)->Release();
      }

      m_lpDirectInput->Release();
      m_lpDirectInput = NULL;
   }
}