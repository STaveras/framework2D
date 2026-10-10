// PlatformMouse.cpp
// GLFW is the windowing/input layer for the OpenGL, Vulkan, and Metal
// renderers on every desktop platform, so this implementation is shared. The
// legacy Win32/DirectX path uses DIMouse instead (see DirectInput.cpp).

#include "PlatformMouse.h"

#include "Window.h"
#include "Maths.h"   // vector2

// The GLFW button for each MouseButton, in MouseButton order.
static const int kGLFWButtons[(int)MouseButton::Count] = {
   GLFW_MOUSE_BUTTON_LEFT,
   GLFW_MOUSE_BUTTON_RIGHT,
   GLFW_MOUSE_BUTTON_MIDDLE,
   GLFW_MOUSE_BUTTON_4,
   GLFW_MOUSE_BUTTON_5,
   GLFW_MOUSE_BUTTON_6,
   GLFW_MOUSE_BUTTON_7,
   GLFW_MOUSE_BUTTON_8,
};

PlatformMouse::PlatformMouse(Window *window) :
   _window(nullptr),
   _cursorHidden(false)
{
   if (window)
   {
      _window = window->getUnderlyingWindow();
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

void PlatformMouse::update(void)
{
   if (!_window)
   {
      return;
   }

   // Button state.
   _buttons.beginFrame();
   for (int button = 0; button < (int)MouseButton::Count; ++button)
   {
      _buttons.set(button, glfwGetMouseButton(_window, kGLFWButtons[button]) == GLFW_PRESS);
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
