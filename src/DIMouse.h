#pragma once

#ifdef _WIN32
#include "DirectInput.h"
#include "Mouse.h"

class DIMouse : public Mouse, IDIDevice
{
   friend class DirectInput;

   HWND _hWnd;
   bool _cursorHidden;
   DIMOUSESTATE2 _mouseState;

   bool _cursorIsInsideClient(void) const;
   bool _syncPositionToClientCursor(void);
   void _setCursorVisibility(bool visible);

public:
   DIMouse(void) {
      _hWnd = NULL;
      _cursorHidden = false;
      ZeroMemory(&_mouseState, sizeof(_mouseState));
   }

   ~DIMouse(void) override;

   bool acquire(LPDIRECTINPUT8 pDI, HWND hWnd = NULL) override;
   void update(void) override;
};
#endif
// NOTE: I'm mimicking how I wrote the DIKeyboard class circa 2010 but this is not the best way to wrap neither
