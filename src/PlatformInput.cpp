
#include "PlatformInput.h"

#include "PlatformKeyboard.h"
#include "PlatformMouse.h"
#include "PlatformGamepad.h"

PlatformInput::PlatformInput(Window *window):_window(window)
{
   _keyboard = new PlatformKeyboard(_window);
   _mouse = new PlatformMouse(_window);
   _gamepad = new PlatformGamepad();
}

PlatformInput::~PlatformInput(void) {
   SAFE_DELETE(_keyboard);
   SAFE_DELETE(_mouse);
   SAFE_DELETE(_gamepad);
}

void PlatformInput::initialize(void)
{
   if (_gamepad)
      _gamepad->update();
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

   if (_gamepad)
      _gamepad->update();
}

void PlatformInput::shutdown(void)
{
   // PlatformKeyboard and PlatformMouse poll GLFW for state and hold no
   // resources to release; nothing to tear down.
}
