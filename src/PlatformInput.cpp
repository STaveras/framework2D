
#include "PlatformInput.h"

#include "PlatformKeyboard.h"

PlatformInput::PlatformInput(Window *window):_window(window)
{
   _keyboard = new PlatformKeyboard(_window);
}

PlatformInput::~PlatformInput(void) {
   SAFE_DELETE(_keyboard);
}

void PlatformInput::Initialize(void)
{
//    if (m_lpDirectInput)
//    {
// 	   if (_keyboard && !((DIKeyboard*)_keyboard)->Acquire(m_lpDirectInput, m_hWnd))
//          throw "Failed to create the keyboard";

//       if (_mouse && !((DIMouse*)_mouse)->Acquire(m_lpDirectInput, m_hWnd))
//          throw "Failed to create the mouse";
//    }
}

void PlatformInput::Update(void)
{
//    if (_keyboard)
//       ((DIKeyboard*)_keyboard)->Update();

//    if (_mouse)
//       ((DIMouse*)_mouse)->Update();
}

void PlatformInput::Shutdown(void)
{
//    if (m_lpDirectInput)
//    {
//       if (_mouse) {
//          ((DIMouse*)_mouse)->Release();
//       }

//       if (_keyboard) {
//          ((DIKeyboard*)_keyboard)->Release();
//       }

//       m_lpDirectInput->Release();
//       m_lpDirectInput = NULL;
//    }
}