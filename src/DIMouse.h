#pragma once

#include "IMouse.h"
#include "DirectInput.h"

class DIMouse : public IMouse, IDIDevice
{
   friend class DirectInput;

   DIMOUSESTATE2 _mouseStateOld;
   DIMOUSESTATE2 _mouseState;

public:
   DIMouse(void) {
      ZeroMemory(&_mouseState, sizeof(_mouseState));
   }

   bool buttonPressed(MOUSE_BUTTONS eBtn) {
      return ((bool)_mouseState.rgbButtons[eBtn] && !(bool)_mouseStateOld.rgbButtons[eBtn]);
   }

   bool buttonReleased(MOUSE_BUTTONS eBtn) {
      return (!(bool)_mouseState.rgbButtons[eBtn] && (bool)_mouseStateOld.rgbButtons[eBtn]);
   }

   bool buttonDown(MOUSE_BUTTONS eBtn) {
      return (bool)_mouseState.rgbButtons[eBtn];
   }

   bool buttonUp(MOUSE_BUTTONS eBtn) {
      return !(bool)_mouseState.rgbButtons[eBtn];
   }

   bool acquire(LPDIRECTINPUT8 pDI, HWND hWnd = NULL);
   void update(void);
};

// NOTE: I'm mimicking how I wrote the DIKeyboard class circa 2010 but this is not the best way to wrap neither