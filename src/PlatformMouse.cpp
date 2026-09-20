// PlatformMouse.cpp
// GLFW-backed mouse for non-Windows platforms (macOS / Linux).

#include "PlatformMouse.h"

#if !defined(_WIN32)

#include "Window.h"
#include "Maths.h"   // vector2

// Map a framework mouse button to a GLFW button so we can read its state via
// glfwGetCursorPos-independent glfwGetMouseButton (window, button).
static int glfwButtonFor(MOUSE_BUTTONS eBtn)
{
   switch (eBtn)
   {
   case MOUSE_LEFT:   return GLFW_MOUSE_BUTTON_LEFT;
   case MOUSE_RIGHT:  return GLFW_MOUSE_BUTTON_RIGHT;
   case MOUSE_MIDDLE: return GLFW_MOUSE_BUTTON_MIDDLE;
   case MOUSE_3:      return GLFW_MOUSE_BUTTON_4;
   case MOUSE_4:      return GLFW_MOUSE_BUTTON_5;
   case MOUSE_5:      return GLFW_MOUSE_BUTTON_6;
   case MOUSE_6:      return GLFW_MOUSE_BUTTON_7;
   case MOUSE_7:      return GLFW_MOUSE_BUTTON_8;
   default:           return -1;
   }
}

PlatformMouse::PlatformMouse(Window *window) :
   _window(nullptr),
   _cursorHidden(false)
{
   if (window)
   {
      _window = window->getUnderlyingWindow();
   }

   for (int i = 0; i < 8; ++i)
   {
      _mouseState[i] = 0;
      _mouseStateOld[i] = 0;
   }

   this->setPosition(0.0f, 0.0f);
}

PlatformMouse::~PlatformMouse(void)
{
   if (_window && _cursorHidden)
   {
      glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
   }
}

bool PlatformMouse::buttonPressed(MOUSE_BUTTONS eBtn)
{
   const int i = (int)eBtn;
   if (i < 0 || i >= 8)
   {
      return false;
   }
   return ((bool)_mouseState[i] && !(bool)_mouseStateOld[i]);
}

bool PlatformMouse::buttonReleased(MOUSE_BUTTONS eBtn)
{
   const int i = (int)eBtn;
   if (i < 0 || i >= 8)
   {
      return false;
   }
   return (!(bool)_mouseState[i] && (bool)_mouseStateOld[i]);
}

bool PlatformMouse::buttonDown(MOUSE_BUTTONS eBtn)
{
   const int i = (int)eBtn;
   if (i < 0 || i >= 8)
   {
      return false;
   }
   return (bool)_mouseState[i];
}

bool PlatformMouse::buttonUp(MOUSE_BUTTONS eBtn)
{
   return !buttonDown(eBtn);
}

void PlatformMouse::update(void)
{
   if (!_window)
   {
      return;
   }

   // Preserve the previous frame's state so edge detection (pressed/released)
   // matches DIMouse's _mouseStateOld / _mouseState pairing.
   for (int i = 0; i < 8; ++i)
   {
      _mouseStateOld[i] = _mouseState[i];
   }

   // Button state.
   for (int e = 0; e < 8; ++e)
   {
      const int glfwBtn = glfwButtonFor((MOUSE_BUTTONS)e);
      if (glfwBtn >= 0)
      {
         _mouseState[e] = (glfwGetMouseButton(_window, glfwBtn) == GLFW_PRESS) ? 1 : 0;
      }
      else
      {
         _mouseState[e] = 0;
      }
   }

   // Cursor position in window/client pixels (origin top-left), matching how
   // DIMouse reports client coordinates.
   double x = 0.0, y = 0.0;
   glfwGetCursorPos(_window, &x, &y);

   vector2 position((float)x, (float)y);

   int width = 0, height = 0;
   glfwGetWindowSize(_window, &width, &height);

   const bool cursorInsideClient =
      x >= 0.0 && y >= 0.0 && x < (double)width && y < (double)height;
   if (cursorInsideClient != _cursorHidden)
   {
      glfwSetInputMode(
         _window,
         GLFW_CURSOR,
         cursorInsideClient ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
      _cursorHidden = cursorInsideClient;
   }

   // Clamp to the client area, mirroring DIMouse::update().
   if (width > 0)
   {
      const float maxX = (float)width;
      if (position.x < 0.0f) position.x = 0.0f;
      else if (position.x > maxX) position.x = maxX;
   }
   if (height > 0)
   {
      const float maxY = (float)height;
      if (position.y < 0.0f) position.y = 0.0f;
      else if (position.y > maxY) position.y = maxY;
   }

   this->setPosition(position);
}

#endif // !defined(_WIN32)
