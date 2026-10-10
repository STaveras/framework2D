// Keyboard.h
// This frame's key state: down/up and the pressed/released edges. A platform
// backend reads its native keys in update() and stores them by Key through
// its KeyMapping table; the queries are shared by every backend.

#pragma once

#include "ButtonState.h"
#include "Key.h"

class Keyboard
{
public:
	virtual ~Keyboard(void) = default;

	bool down(Key key) const { return _keys.down((int)key); }
	bool up(Key key) const { return _keys.up((int)key); }
	bool pressed(Key key) const { return _keys.pressed((int)key); }
	bool released(Key key) const { return _keys.released((int)key); }

	virtual void update(void) = 0;

protected:
	ButtonStateSet _keys{ (size_t)Key::Count };
};
