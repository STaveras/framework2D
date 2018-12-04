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
      throw "DIMouse::ButtonPressed unimplemented";
   }

   bool ButtonReleased(MOUSE_BUTTONS eBtn) {
      throw "DIMouse::ButtonReleased unimplemented";
   }

   bool ButtonDown(MOUSE_BUTTONS eBtn) {
      throw "DIMouse::ButtonDown unimplemented";
   }

   bool ButtonUp(MOUSE_BUTTONS eBtn) {
      throw "DIMouse::ButtonUp unimplemented";
   }

   void Update(void) {

      IDIDeviceComm::Update();

      memcpy_s(&_mouseStateOld, sizeof(DIMOUSESTATE2), &_mouseState, sizeof(DIMOUSESTATE2));

      if (SUCCEEDED(m_lpDevice->Poll())) {
         if (m_lpDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &_mouseState) == DIERR_INPUTLOST) {
            m_bDeviceLost = true;
         }
         else {
            this->SetPosition(vector2(_mouseState.lX, _mouseState.lY));
         }
      }

   }
};

// NOTE: I'm mimicking how I wrote the DIKeyboard class circa 2010 but this is not the best way to wrap either