// PlatformMouse.h
// GLFW implementation of the Mouse interface for GLFW-backed windows.
//
// This is the cross-platform equivalent of the Windows DIMouse. It polls
// GLFW each frame for the cursor position (window/client pixel coordinates,
// origin at top-left) and button state, exposing them through the same IMouse
// contract used by the Windows implementation:
//   - Position: Positionable::getPosition() / _position / _x / _y
//   - Buttons:  buttonPressed / buttonReleased / buttonDown / buttonUp
//
// Position is clamped to the window's client area, mirroring DIMouse::update().

#pragma once

#include "IMouse.h"

class Window;

class PlatformMouse : public IMouse
{
   GLFWwindow* _window;
   bool _cursorHidden;

   // Per-button down state for the current and previous frame, indexed by the
   // framework's MOUSE_BUTTONS enum (mirrors DIMouse rgbButtons[eBtn]).
   unsigned char _mouseState[8];
   unsigned char _mouseStateOld[8];

public:
   explicit PlatformMouse(Window *window);
   ~PlatformMouse(void);

   // Edge / level button state, matching DIMouse semantics.
   bool buttonPressed(MOUSE_BUTTONS eBtn) override;
   bool buttonReleased(MOUSE_BUTTONS eBtn) override;
   bool buttonDown(MOUSE_BUTTONS eBtn) override;
   bool buttonUp(MOUSE_BUTTONS eBtn) override;

   // Poll GLFW for cursor position and button state.
   void update(void);
};
