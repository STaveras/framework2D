
#include "PlatformInput.h"

#include "PlatformKeyboard.h"
#include "PlatformMouse.h"

PlatformInput::PlatformInput(Window *window):_window(window)
{
   _keyboard = new PlatformKeyboard(_window);
   _mouse = new PlatformMouse(_window);
}

PlatformInput::~PlatformInput(void) {
   SAFE_DELETE(_keyboard);
   SAFE_DELETE(_mouse);
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
   if (_keyboard)
      _keyboard->update();

   if (_mouse)
      ((PlatformMouse*)_mouse)->update();
}

void PlatformInput::shutdown(void)
{
   // PlatformKeyboard and PlatformMouse poll GLFW for state and hold no
   // resources to release; nothing to tear down.
}