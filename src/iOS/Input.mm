// Input.mm
#if !__has_feature(objc_arc)
#error "Input.mm must be compiled with -fobjc-arc"
#endif

// System frameworks first: GameController names an Objective-C generic
// parameter Key, which would clash with the engine's Key if declared earlier.
#import <GameController/GameController.h>
#import <UIKit/UIKit.h>

#include "Input.h"

#include <cmath>
#include <vector>

namespace {
const KeyMapping kHIDKeys[] = {
	{ Key::Escape, UIKeyboardHIDUsageKeyboardEscape },
	{ Key::Num0, UIKeyboardHIDUsageKeyboard0 },
	{ Key::Num1, UIKeyboardHIDUsageKeyboard1 },
	{ Key::Num2, UIKeyboardHIDUsageKeyboard2 },
	{ Key::Num3, UIKeyboardHIDUsageKeyboard3 },
	{ Key::Num4, UIKeyboardHIDUsageKeyboard4 },
	{ Key::Num5, UIKeyboardHIDUsageKeyboard5 },
	{ Key::Num6, UIKeyboardHIDUsageKeyboard6 },
	{ Key::Num7, UIKeyboardHIDUsageKeyboard7 },
	{ Key::Num8, UIKeyboardHIDUsageKeyboard8 },
	{ Key::Num9, UIKeyboardHIDUsageKeyboard9 },
	{ Key::A, UIKeyboardHIDUsageKeyboardA },
	{ Key::B, UIKeyboardHIDUsageKeyboardB },
	{ Key::C, UIKeyboardHIDUsageKeyboardC },
	{ Key::D, UIKeyboardHIDUsageKeyboardD },
	{ Key::E, UIKeyboardHIDUsageKeyboardE },
	{ Key::F, UIKeyboardHIDUsageKeyboardF },
	{ Key::G, UIKeyboardHIDUsageKeyboardG },
	{ Key::H, UIKeyboardHIDUsageKeyboardH },
	{ Key::I, UIKeyboardHIDUsageKeyboardI },
	{ Key::J, UIKeyboardHIDUsageKeyboardJ },
	{ Key::K, UIKeyboardHIDUsageKeyboardK },
	{ Key::L, UIKeyboardHIDUsageKeyboardL },
	{ Key::M, UIKeyboardHIDUsageKeyboardM },
	{ Key::N, UIKeyboardHIDUsageKeyboardN },
	{ Key::O, UIKeyboardHIDUsageKeyboardO },
	{ Key::P, UIKeyboardHIDUsageKeyboardP },
	{ Key::Q, UIKeyboardHIDUsageKeyboardQ },
	{ Key::R, UIKeyboardHIDUsageKeyboardR },
	{ Key::S, UIKeyboardHIDUsageKeyboardS },
	{ Key::T, UIKeyboardHIDUsageKeyboardT },
	{ Key::U, UIKeyboardHIDUsageKeyboardU },
	{ Key::V, UIKeyboardHIDUsageKeyboardV },
	{ Key::W, UIKeyboardHIDUsageKeyboardW },
	{ Key::X, UIKeyboardHIDUsageKeyboardX },
	{ Key::Y, UIKeyboardHIDUsageKeyboardY },
	{ Key::Z, UIKeyboardHIDUsageKeyboardZ },
	{ Key::F1, UIKeyboardHIDUsageKeyboardF1 },
	{ Key::F2, UIKeyboardHIDUsageKeyboardF2 },
	{ Key::F3, UIKeyboardHIDUsageKeyboardF3 },
	{ Key::F4, UIKeyboardHIDUsageKeyboardF4 },
	{ Key::F5, UIKeyboardHIDUsageKeyboardF5 },
	{ Key::F6, UIKeyboardHIDUsageKeyboardF6 },
	{ Key::F7, UIKeyboardHIDUsageKeyboardF7 },
	{ Key::F8, UIKeyboardHIDUsageKeyboardF8 },
	{ Key::F9, UIKeyboardHIDUsageKeyboardF9 },
	{ Key::F10, UIKeyboardHIDUsageKeyboardF10 },
	{ Key::F11, UIKeyboardHIDUsageKeyboardF11 },
	{ Key::F12, UIKeyboardHIDUsageKeyboardF12 },
	{ Key::F13, UIKeyboardHIDUsageKeyboardF13 },
	{ Key::F14, UIKeyboardHIDUsageKeyboardF14 },
	{ Key::F15, UIKeyboardHIDUsageKeyboardF15 },
	{ Key::Space, UIKeyboardHIDUsageKeyboardSpacebar },
	{ Key::Enter, UIKeyboardHIDUsageKeyboardReturnOrEnter },
	{ Key::Tab, UIKeyboardHIDUsageKeyboardTab },
	{ Key::Backspace, UIKeyboardHIDUsageKeyboardDeleteOrBackspace },
	{ Key::Insert, UIKeyboardHIDUsageKeyboardInsert },
	{ Key::Delete, UIKeyboardHIDUsageKeyboardDeleteForward },
	{ Key::Home, UIKeyboardHIDUsageKeyboardHome },
	{ Key::End, UIKeyboardHIDUsageKeyboardEnd },
	{ Key::PageUp, UIKeyboardHIDUsageKeyboardPageUp },
	{ Key::PageDown, UIKeyboardHIDUsageKeyboardPageDown },
	{ Key::Up, UIKeyboardHIDUsageKeyboardUpArrow },
	{ Key::Down, UIKeyboardHIDUsageKeyboardDownArrow },
	{ Key::Left, UIKeyboardHIDUsageKeyboardLeftArrow },
	{ Key::Right, UIKeyboardHIDUsageKeyboardRightArrow },
	{ Key::Minus, UIKeyboardHIDUsageKeyboardHyphen },
	{ Key::Equals, UIKeyboardHIDUsageKeyboardEqualSign },
	{ Key::LeftBracket, UIKeyboardHIDUsageKeyboardOpenBracket },
	{ Key::RightBracket, UIKeyboardHIDUsageKeyboardCloseBracket },
	{ Key::Backslash, UIKeyboardHIDUsageKeyboardBackslash },
	{ Key::Semicolon, UIKeyboardHIDUsageKeyboardSemicolon },
	{ Key::Apostrophe, UIKeyboardHIDUsageKeyboardQuote },
	{ Key::Grave, UIKeyboardHIDUsageKeyboardGraveAccentAndTilde },
	{ Key::Comma, UIKeyboardHIDUsageKeyboardComma },
	{ Key::Period, UIKeyboardHIDUsageKeyboardPeriod },
	{ Key::Slash, UIKeyboardHIDUsageKeyboardSlash },
	{ Key::LeftShift, UIKeyboardHIDUsageKeyboardLeftShift },
	{ Key::RightShift, UIKeyboardHIDUsageKeyboardRightShift },
	{ Key::LeftControl, UIKeyboardHIDUsageKeyboardLeftControl },
	{ Key::RightControl, UIKeyboardHIDUsageKeyboardRightControl },
	{ Key::LeftAlt, UIKeyboardHIDUsageKeyboardLeftAlt },
	{ Key::RightAlt, UIKeyboardHIDUsageKeyboardRightAlt },
	{ Key::LeftSuper, UIKeyboardHIDUsageKeyboardLeftGUI },
	{ Key::RightSuper, UIKeyboardHIDUsageKeyboardRightGUI },
	{ Key::Menu, UIKeyboardHIDUsageKeyboardApplication },
	{ Key::CapsLock, UIKeyboardHIDUsageKeyboardCapsLock },
	{ Key::NumLock, UIKeyboardHIDUsageKeypadNumLock },
	{ Key::ScrollLock, UIKeyboardHIDUsageKeyboardScrollLock },
	{ Key::PrintScreen, UIKeyboardHIDUsageKeyboardPrintScreen },
	{ Key::Pause, UIKeyboardHIDUsageKeyboardPause },
	{ Key::Keypad0, UIKeyboardHIDUsageKeypad0 },
	{ Key::Keypad1, UIKeyboardHIDUsageKeypad1 },
	{ Key::Keypad2, UIKeyboardHIDUsageKeypad2 },
	{ Key::Keypad3, UIKeyboardHIDUsageKeypad3 },
	{ Key::Keypad4, UIKeyboardHIDUsageKeypad4 },
	{ Key::Keypad5, UIKeyboardHIDUsageKeypad5 },
	{ Key::Keypad6, UIKeyboardHIDUsageKeypad6 },
	{ Key::Keypad7, UIKeyboardHIDUsageKeypad7 },
	{ Key::Keypad8, UIKeyboardHIDUsageKeypad8 },
	{ Key::Keypad9, UIKeyboardHIDUsageKeypad9 },
	{ Key::KeypadDecimal, UIKeyboardHIDUsageKeypadPeriod },
	{ Key::KeypadDivide, UIKeyboardHIDUsageKeypadSlash },
	{ Key::KeypadMultiply, UIKeyboardHIDUsageKeypadAsterisk },
	{ Key::KeypadSubtract, UIKeyboardHIDUsageKeypadHyphen },
	{ Key::KeypadAdd, UIKeyboardHIDUsageKeypadPlus },
	{ Key::KeypadEnter, UIKeyboardHIDUsageKeypadEnter },
	{ Key::KeypadEquals, UIKeyboardHIDUsageKeypadEqualSign },
};

void readButtons(GCExtendedGamepad* gamepad, bool* down)
{
	using Button = Gamepad::Button;
	down[(int)Button::A] = gamepad.buttonA.isPressed;
	down[(int)Button::B] = gamepad.buttonB.isPressed;
	down[(int)Button::X] = gamepad.buttonX.isPressed;
	down[(int)Button::Y] = gamepad.buttonY.isPressed;
	down[(int)Button::LeftBumper] = gamepad.leftShoulder.isPressed;
	down[(int)Button::RightBumper] = gamepad.rightShoulder.isPressed;
	down[(int)Button::Back] = gamepad.buttonOptions.isPressed;
	down[(int)Button::Start] = gamepad.buttonMenu.isPressed;
	down[(int)Button::Guide] = gamepad.buttonHome.isPressed;
	down[(int)Button::LeftThumb] = gamepad.leftThumbstickButton.isPressed;
	down[(int)Button::RightThumb] = gamepad.rightThumbstickButton.isPressed;
	down[(int)Button::DpadUp] = gamepad.dpad.up.isPressed;
	down[(int)Button::DpadRight] = gamepad.dpad.right.isPressed;
	down[(int)Button::DpadDown] = gamepad.dpad.down.isPressed;
	down[(int)Button::DpadLeft] = gamepad.dpad.left.isPressed;
}
}

IOSKeyboard::~IOSKeyboard(void)
{
	GCKeyboard* keyboard = GCKeyboard.coalescedKeyboard;
	if (keyboard && (__bridge const void*)keyboard == _attachedKeyboard) {
		keyboard.keyboardInput.keyChangedHandler = nil;
	}
}

void IOSKeyboard::onKeyChanged(int hidUsage, bool pressed)
{
	const Key key = findKey(kHIDKeys, hidUsage);
	if (key == Key::Unknown) {
		return;
	}
	_held[(size_t)key] = pressed;
	// Latched so a press and release between two updates still reads as down
	if (pressed) {
		_keys.latchPress((int)key);
	}
}

void IOSKeyboard::update(void)
{
	// Keyboards come and go; follow whichever one is current.
	GCKeyboard* keyboard = GCKeyboard.coalescedKeyboard;
	if ((__bridge const void*)keyboard != _attachedKeyboard) {
		_held.fill(false);
		_attachedKeyboard = (__bridge const void*)keyboard;
		IOSKeyboard* owner = this;
		keyboard.keyboardInput.keyChangedHandler = ^(GCKeyboardInput*, GCControllerButtonInput*, GCKeyCode keyCode, BOOL pressed) {
			owner->onKeyChanged((int)keyCode, pressed);
		};
	}

	_keys.beginFrame();
	for (size_t key = 0; key < _held.size(); ++key) {
		_keys.set((int)key, _held[key]);
	}
}

void IOSGamepad::update(void)
{
	std::vector<Reading> readings;

	for (GCController* controller in GCController.controllers) {
		GCExtendedGamepad* gamepad = controller.extendedGamepad;
		if (!gamepad) {
			continue;
		}

		Reading& reading = readings.emplace_back();
		reading.name = controller.vendorName ? controller.vendorName.UTF8String : "Game controller";
		reading.guid = reading.name;
		readButtons(gamepad, reading.down.data());
		// GameController's y axes point up; the engine's (GLFW's) point down.
		reading.axes[(size_t)Axis::LeftX] = gamepad.leftThumbstick.xAxis.value;
		reading.axes[(size_t)Axis::LeftY] = -gamepad.leftThumbstick.yAxis.value;
		reading.axes[(size_t)Axis::RightX] = gamepad.rightThumbstick.xAxis.value;
		reading.axes[(size_t)Axis::RightY] = -gamepad.rightThumbstick.yAxis.value;
		reading.axes[(size_t)Axis::LeftTrigger] = gamepad.leftTrigger.value;
		reading.axes[(size_t)Axis::RightTrigger] = gamepad.rightTrigger.value;
	}

	// The touch controls play as the first pad, alongside any controller there.
	if (_touch && _touch->active) {
		if (readings.empty()) {
			Reading& touch = readings.emplace_back();
			touch.name = touch.guid = "Touch controls";
		}
		Reading& first = readings.front();
		for (size_t button = 0; button < first.down.size(); ++button) {
			first.down[button] = first.down[button] || _touch->down[button];
			first.tapped[button] = _touch->tapped[button];
		}
		_touch->tapped.fill(false);

		// Whichever stick is pushed further wins.
		float& x = first.axes[(size_t)Axis::LeftX];
		float& y = first.axes[(size_t)Axis::LeftY];
		if (std::hypot(_touch->leftX, _touch->leftY) > std::hypot(x, y)) {
			x = _touch->leftX;
			y = _touch->leftY;
		}
	}

	_setReadings(readings);
}

IOSInput::IOSInput(TouchGamepadState* touch)
{
	// No mouse: touches go to the on-screen controls, not a pointer.
	_keyboard = std::make_unique<IOSKeyboard>();
	_gamepad = std::make_unique<IOSGamepad>(touch);
}
