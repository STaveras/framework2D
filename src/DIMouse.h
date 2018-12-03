#pragma once

#include "IMouse.h"

#include "MOUSE_BUTTONS.h"

class DIMouse : public IMouse, IDIDeviceComm
{
   friend class DirectInput;

public:
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
};

// NOTE: I'm mimicking how I wrote the DIKeyboard class circa 2010 but this is not the best way to wrap either