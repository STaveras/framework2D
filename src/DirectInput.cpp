
#ifdef _WIN32

#include "DirectInput.h"

#include "KeyboardDI.h"
#include "DIMouse.h"
#include "PlatformGamepad.h"

DirectInput::DirectInput(HINSTANCE hInstance, HWND hWnd) :
   m_hWnd(hWnd),
   m_lpDirectInput(NULL) {

   _keyboard = new KeyboardDI();
   _mouse = new DIMouse();
   _gamepad = new PlatformGamepad();

   if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_lpDirectInput, NULL)))
      throw std::runtime_error("Failed to create DirectInput COM interface");
}

DirectInput::~DirectInput(void) {
   
   SAFE_DELETE(_keyboard);
   SAFE_DELETE(_mouse);
   SAFE_DELETE(_gamepad);
   
   // if (m_lpDirectInput) {
   //    if (_mouse) {
   //       ((DIMouse*)_mouse)->release();
   //    }

   //    if (_keyboard) {
   //       ((DIKeyboard*)_keyboard)->release();
   //    }

   //    m_lpDirectInput->release();
   //    m_lpDirectInput = NULL;
   // }
}

void DirectInput::initialize(void)
{
   // We should honestly just do all this stuff in the constructor
   if (m_lpDirectInput)
   {
	   if (_keyboard && !((KeyboardDI*)_keyboard)->acquire(m_lpDirectInput, m_hWnd))
         throw std::runtime_error("Failed to create the keyboard");

      if (_mouse && !((DIMouse*)_mouse)->acquire(m_lpDirectInput, m_hWnd))
         throw std::runtime_error("Failed to create the mouse");
   }

   //Keyboard::KEYS::
}

void DirectInput::update(void)
{
   if (_keyboard)
      ((KeyboardDI*)_keyboard)->update();

   if (_mouse)
      ((DIMouse*)_mouse)->update();

   if (_gamepad)
      _gamepad->update();
}

void DirectInput::shutdown(void)
{
   if (m_lpDirectInput)
   {
      if (_mouse) {
         ((DIMouse*)_mouse)->release();
      }

      if (_keyboard) {
         ((KeyboardDI*)_keyboard)->release();
      }

      m_lpDirectInput->Release();
      m_lpDirectInput = NULL;
   }
}

#endif
