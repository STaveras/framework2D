// Mouse.h
// The pointer's position in client-area pixels (origin top-left) and this
// frame's button state. A platform backend reads both in update(); the button
// queries are shared by every backend.

#pragma once

#include "ButtonState.h"
#include "Positionable.h"

enum class MouseButton
{
	Left,
	Right,
	Middle,
	Button4,
	Button5,
	Button6,
	Button7,
	Button8,
	Count
};

class Mouse : public Positionable
{
public:
	virtual ~Mouse(void) = default;

	bool down(MouseButton button) const { return _buttons.down((int)button); }
	bool up(MouseButton button) const { return _buttons.up((int)button); }
	bool pressed(MouseButton button) const { return _buttons.pressed((int)button); }
	bool released(MouseButton button) const { return _buttons.released((int)button); }

	virtual void update(void) = 0;

protected:
	ButtonStateSet _buttons{ (size_t)MouseButton::Count };
};
