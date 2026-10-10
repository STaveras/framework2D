// Input.h
// Input on iOS through the GameController framework: hardware keyboards
// (GCKeyboard), game controllers (GCController extended gamepads), and the
// on-screen touch controls as a virtual gamepad that stands in while no
// controller is connected. There is no mouse; getMouse() returns NULL.

#pragma once

#include "../IInput.h"

#include <array>

// Shared by the touch controls (which write it) and IOSGamepad (which reads it
// once per input update). Both run on the main thread.
struct TouchGamepadState
{
	static constexpr int kButtonCount = Gamepad::kButtonCount;

	bool active = false; // the controls are on screen
	std::array<bool, kButtonCount> down{};
	// Presses since the last poll, so a tap shorter than a frame still registers.
	std::array<bool, kButtonCount> tapped{};
	float leftX = 0.0f; // -1 (left) to 1 (right)
	float leftY = 0.0f; // -1 (up) to 1 (down), like GLFW

	void release(void)
	{
		down.fill(false);
		tapped.fill(false);
		leftX = leftY = 0.0f;
	}
};

// Keys arrive as USB HID usages (GCKeyCode / UIKeyboardHIDUsage).
class IOSKeyboard : public Keyboard
{
	std::array<bool, (size_t)Key::Count> _held{};
	const void* _attachedKeyboard = nullptr; // GCKeyboard whose handler feeds _held

public:
	~IOSKeyboard(void) override;

	void update(void) override;

	// Called from the GCKeyboard handler
	void onKeyChanged(int hidUsage, bool pressed);
};

// Pads are the connected extended gamepads in GCController order. While the
// touch controls are on screen they play as the first pad: on their own when
// no controller is connected, otherwise combined with the first controller.
class IOSGamepad : public Gamepad
{
	TouchGamepadState* _touch;

public:
	explicit IOSGamepad(TouchGamepadState* touch) : _touch(touch) {}

	void update(void) override;
};

class IOSInput : public IInput
{
public:
	explicit IOSInput(TouchGamepadState* touch);

	void initialize(void) override { update(); }
};
