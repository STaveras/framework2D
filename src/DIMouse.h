#pragma once

#include "IMouse.h"
#include "DirectInput.h"

class DIMouse : public IMouse, IDIDeviceComm
{
   friend class DirectInput;

   DIMOUSESTATE2 _mouseStateOld;
   DIMOUSESTATE2 _mouseState;

public:
   DIMouse(void) {
      ZeroMemory(&_mouseState, sizeof(_mouseState));
   }

   bool ButtonPressed(MOUSE_BUTTONS eBtn) {
      return ((bool)_mouseState.rgbButtons[eBtn] && !(bool)_mouseStateOld.rgbButtons[eBtn]);
   }

   bool ButtonReleased(MOUSE_BUTTONS eBtn) {
      return (!(bool)_mouseState.rgbButtons[eBtn] && (bool)_mouseStateOld.rgbButtons[eBtn]);
   }

   bool ButtonDown(MOUSE_BUTTONS eBtn) {
      return (bool)_mouseState.rgbButtons[eBtn];
   }

   bool ButtonUp(MOUSE_BUTTONS eBtn) {
      return !(bool)_mouseState.rgbButtons[eBtn];
   }

   bool Acquire(LPDIRECTINPUT8 pDI, HWND hWnd = NULL);
   void Update(void);
};

// NOTE: I'm mimicking how I wrote the DIKeyboard class circa 2010 but this is not the best way to wrap neither