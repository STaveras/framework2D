
#ifdef _WIN32

#include "DirectInput.h"

#include "KeyboardDI.h"
#include "DIMouse.h"
#include "PlatformGamepad.h"

DirectInput::DirectInput(HINSTANCE hInstance, HWND hWnd) :
   m_hWnd(hWnd),
   m_lpDirectInput(NULL) {

   _keyboard = std::make_unique<KeyboardDI>();
   _mouse = std::make_unique<DIMouse>();
   _gamepad = std::make_unique<PlatformGamepad>();

   if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_lpDirectInput, NULL)))
      throw std::runtime_error("Failed to create DirectInput COM interface");
}

void DirectInput::initialize(void)
{
   // We should honestly just do all this stuff in the constructor
   if (m_lpDirectInput)
   {
	   if (_keyboard && !static_cast<KeyboardDI*>(_keyboard.get())->acquire(m_lpDirectInput, m_hWnd))
         throw std::runtime_error("Failed to create the keyboard");

      if (_mouse && !static_cast<DIMouse*>(_mouse.get())->acquire(m_lpDirectInput, m_hWnd))
         throw std::runtime_error("Failed to create the mouse");
   }
}

void DirectInput::shutdown(void)
{
   if (m_lpDirectInput)
   {
      if (_mouse) {
         static_cast<DIMouse*>(_mouse.get())->release();
      }

      if (_keyboard) {
         static_cast<KeyboardDI*>(_keyboard.get())->release();
      }

      m_lpDirectInput->Release();
      m_lpDirectInput = NULL;
   }
}

#endif
