// Input.h
// Input on iOS through the GameController framework: hardware keyboards
// (GCKeyboard), game controllers (GCController extended gamepads), and the
// on-screen touch controls as a virtual gamepad that stands in while no
// controller is connected. There is no mouse; getMouse() returns NULL.

#pragma once

#include "../ButtonState.h"
#include "../IInput.h"

#include <array>
#include <string>
#include <vector>

// Shared by the touch controls (which write it) and IOSGamepad (which reads it
// once per input update). Both run on the main thread.
struct TouchGamepadState
{
	static constexpr int kButtonCount = (int)Gamepad::Button::DpadLeft + 1;

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

// Key codes are USB HID usages (GCKeyCode / UIKeyboardHIDUsage).
class IOSKeyboard : public Keyboard
{
	static constexpr int kKeyCount = 256;
	ButtonStateSet _keys{ kKeyCount };
	std::array<bool, kKeyCount> _held{};
	const void* _attachedKeyboard = nullptr; // GCKeyboard whose handler feeds _held

public:
	IOSKeyboard(void);
	~IOSKeyboard(void) override;

	bool keyDown(KEY key) override { return _keys.down(key); }
	bool keyUp(KEY key) override { return _keys.up(key); }
	bool keyPressed(KEY key) override { return _keys.pressed(key); }
	bool keyReleased(KEY key) override { return _keys.released(key); }

	void update(void) override;

	// Called from the GCKeyboard handler
	void onKeyChanged(int key, bool pressed);

	static const KEYS kHIDKeys;
	const KEYS& getKeys(void) const override { return kHIDKeys; }
};

// Pads are the connected extended gamepads in GCController order. While the
// touch controls are on screen they play as the first pad: on their own when
// no controller is connected, otherwise combined with the first controller.
class IOSGamepad : public Gamepad
{
	static constexpr int kAxisCount = (int)Axis::RightTrigger + 1;

	struct Pad
	{
		std::string name;
		ButtonStateSet buttons{ TouchGamepadState::kButtonCount };
		std::array<float, kAxisCount> axes{};
		std::array<float, kAxisCount> previousAxes{};
		bool connected = false;
	};

	TouchGamepadState* _touch;
	std::vector<Pad> _pads;
	int _connected = 0;

	const Pad* _pad(int padIndex) const;

public:
	explicit IOSGamepad(TouchGamepadState* touch);

	int getConnectedCount(void) const override { return _connected; }
	bool isConnected(int padIndex = 0) const override { return _pad(padIndex) != nullptr; }
	std::string getName(int padIndex = 0) const override;
	std::string getGUID(int padIndex = 0) const override { return getName(padIndex); }

	bool buttonDown(Button button, int padIndex = 0) const override;
	bool buttonUp(Button button, int padIndex = 0) const override { return !buttonDown(button, padIndex); }
	bool buttonPressed(Button button, int padIndex = 0) const override;
	bool buttonReleased(Button button, int padIndex = 0) const override;

	float getAxis(Axis axis, int padIndex = 0) const override;
	bool axisDown(Axis axis, float threshold, int padIndex = 0) const override;
	bool axisPressed(Axis axis, float threshold, int padIndex = 0) const override;
	bool axisReleased(Axis axis, float threshold, int padIndex = 0) const override;

	void update(void) override;
};

class IOSInput : public IInput
{
public:
	explicit IOSInput(TouchGamepadState* touch);
	~IOSInput(void) override;

	void initialize(void) override;
	void update(void) override;
	void shutdown(void) override;
};
