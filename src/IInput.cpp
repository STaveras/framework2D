#include "IInput.h"

void IInput::update(void)
{
	if (_keyboard) {
		_keyboard->update();
	}
	if (_mouse) {
		_mouse->update();
	}
	if (_gamepad) {
		_gamepad->update();
	}
}
