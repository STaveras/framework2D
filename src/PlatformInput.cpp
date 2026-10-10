#include "PlatformInput.h"

#include "PlatformGamepad.h"
#include "PlatformKeyboard.h"
#include "PlatformMouse.h"

PlatformInput::PlatformInput(Window* window)
{
	_keyboard = std::make_unique<PlatformKeyboard>(window);
	_mouse = std::make_unique<PlatformMouse>(window);
	_gamepad = std::make_unique<PlatformGamepad>();
}

void PlatformInput::initialize(void)
{
	// Find the pads already connected before the first frame.
	if (_gamepad) {
		_gamepad->update();
	}
}
