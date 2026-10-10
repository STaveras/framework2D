// File: IInput.h
// The input devices of one platform: a keyboard, a mouse and gamepads, any of
// which may be missing (iOS has no mouse). A platform subclass creates them
// into these owning pointers; this base polls them once per frame in update().

#pragma once

#include "Types.h"

#include "Gamepad.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <memory>

class IInput
{
protected:
	std::unique_ptr<Keyboard> _keyboard;
	std::unique_ptr<Mouse> _mouse;
	std::unique_ptr<Gamepad> _gamepad;

public:
	IInput(void) = default;
	IInput(const IInput&) = delete;
	IInput& operator=(const IInput&) = delete;
	virtual ~IInput(void) = default;

	Keyboard* getKeyboard(void) const { return _keyboard.get(); }
	Mouse* getMouse(void) const { return _mouse.get(); }
	Gamepad* getGamepad(void) const { return _gamepad.get(); }

	virtual void initialize(void) {}
	// Poll every device for this frame's state.
	virtual void update(void);
	virtual void shutdown(void) {}
};
