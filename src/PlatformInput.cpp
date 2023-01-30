
#include "PlatformInput.h"

#include "PlatformKeyboard.h"

PlatformInput::PlatformInput(Window *window):_window(window)
{
   _keyboard = new PlatformKeyboard(_window);
}

PlatformInput::~PlatformInput(void) {
   SAFE_DELETE(_keyboard);
}

void PlatformInput::initialize(void)
{
//    if (m_lpDirectInput)
//    {
// 	   if (_keyboard && !((DIKeyboard*)_keyboard)->Acquire(m_lpDirectInput, m_hWnd))
//          throw "Failed to create the keyboard";

//       if (_mouse && !((DIMouse*)_mouse)->Acquire(m_lpDirectInput, m_hWnd))
//          throw "Failed to create the mouse";
//    }
}

void PlatformInput::update(void)
{
//    if (_keyboard)
//       ((DIKeyboard*)_keyboard)->update();

//    if (_mouse)
//       ((DIMouse*)_mouse)->update();
}

void PlatformInput::shutdown(void)
{
//    if (m_lpDirectInput)
//    {
//       if (_mouse) {
//          ((DIMouse*)_mouse)->release();
//       }

//       if (_keyboard) {
//          ((DIKeyboard*)_keyboard)->release();
//       }

//       m_lpDirectInput->release();
//       m_lpDirectInput = NULL;
//    }
}