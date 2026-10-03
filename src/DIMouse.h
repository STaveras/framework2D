#pragma once

#ifdef _WIN32
#include "IMouse.h"
#include "DirectInput.h"
#include "ButtonState.h"

class DIMouse : public IMouse, IDIDevice
{
   friend class DirectInput;

   HWND _hWnd;
   bool _cursorHidden;
   DIMOUSESTATE2 _mouseState;
   // Indexed by MOUSE_BUTTONS, fed from _mouseState.rgbButtons in update().
   ButtonStateSet _buttons{8};

   bool _cursorIsInsideClient(void) const;
   bool _syncPositionToClientCursor(void);
   void _setCursorVisibility(bool visible);

public:
   DIMouse(void) {
      _hWnd = NULL;
      _cursorHidden = false;
      ZeroMemory(&_mouseState, sizeof(_mouseState));
   }

   ~DIMouse(void);

   bool buttonPressed(MOUSE_BUTTONS eBtn) { return _buttons.pressed((int)eBtn); }
   bool buttonReleased(MOUSE_BUTTONS eBtn) { return _buttons.released((int)eBtn); }
   bool buttonDown(MOUSE_BUTTONS eBtn) { return _buttons.down((int)eBtn); }
   bool buttonUp(MOUSE_BUTTONS eBtn) { return _buttons.up((int)eBtn); }

   bool acquire(LPDIRECTINPUT8 pDI, HWND hWnd = NULL);
   void update(void);
};
#endif
// NOTE: I'm mimicking how I wrote the DIKeyboard class circa 2010 but this is not the best way to wrap neither
