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
   // DIMouse reports client coordinates. It is not clamped: GLFW keeps
   // reporting the position past the window's edges, so the in-game cursor
   // slides off-screen with the hidden OS cursor instead of pinning to an edge.
   double x = 0.0, y = 0.0;
   glfwGetCursorPos(_window, &x, &y);

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

   this->setPosition(vector2((float)x, (float)y));
}
