// Input.mm
#if !__has_feature(objc_arc)
#error "Input.mm must be compiled with -fobjc-arc"
#endif

#include "Input.h"

#import <GameController/GameController.h>
#import <UIKit/UIKit.h>

#include <algorithm>
#include <cmath>

// Same field order as IKeyboard::KEYS; 0 where a key has no HID usage.
const IOSKeyboard::KEYS IOSKeyboard::kHIDKeys{
	UIKeyboardHIDUsageKeyboardEscape,
	UIKeyboardHIDUsageKeyboard1,
	UIKeyboardHIDUsageKeyboard2,
	UIKeyboardHIDUsageKeyboard3,
	UIKeyboardHIDUsageKeyboard4,
	UIKeyboardHIDUsageKeyboard5,
	UIKeyboardHIDUsageKeyboard6,
	UIKeyboardHIDUsageKeyboard7,
	UIKeyboardHIDUsageKeyboard8,
	UIKeyboardHIDUsageKeyboard9,
	UIKeyboardHIDUsageKeyboard0,
	UIKeyboardHIDUsageKeyboardHyphen,
	UIKeyboardHIDUsageKeyboardEqualSign,
	UIKeyboardHIDUsageKeyboardDeleteOrBackspace,
	UIKeyboardHIDUsageKeyboardTab,
	UIKeyboardHIDUsageKeyboardQ,
	UIKeyboardHIDUsageKeyboardW,
	UIKeyboardHIDUsageKeyboardE,
	UIKeyboardHIDUsageKeyboardR,
	UIKeyboardHIDUsageKeyboardT,
	UIKeyboardHIDUsageKeyboardY,
	UIKeyboardHIDUsageKeyboardU,
	UIKeyboardHIDUsageKeyboardI,
	UIKeyboardHIDUsageKeyboardO,
	UIKeyboardHIDUsageKeyboardP,
	UIKeyboardHIDUsageKeyboardOpenBracket,
	UIKeyboardHIDUsageKeyboardCloseBracket,
	UIKeyboardHIDUsageKeyboardReturnOrEnter,
	UIKeyboardHIDUsageKeyboardLeftControl,
	UIKeyboardHIDUsageKeyboardA,
	UIKeyboardHIDUsageKeyboardS,
	UIKeyboardHIDUsageKeyboardD,
	UIKeyboardHIDUsageKeyboardF,
	UIKeyboardHIDUsageKeyboardG,
	UIKeyboardHIDUsageKeyboardH,
	UIKeyboardHIDUsageKeyboardJ,
	UIKeyboardHIDUsageKeyboardK,
	UIKeyboardHIDUsageKeyboardL,
	UIKeyboardHIDUsageKeyboardSemicolon,
	UIKeyboardHIDUsageKeyboardQuote,
	UIKeyboardHIDUsageKeyboardGraveAccentAndTilde,
	UIKeyboardHIDUsageKeyboardLeftShift,
	UIKeyboardHIDUsageKeyboardBackslash,
	UIKeyboardHIDUsageKeyboardZ,
	UIKeyboardHIDUsageKeyboardX,
	UIKeyboardHIDUsageKeyboardC,
	UIKeyboardHIDUsageKeyboardV,
	UIKeyboardHIDUsageKeyboardB,
	UIKeyboardHIDUsageKeyboardN,
	UIKeyboardHIDUsageKeyboardM,
	UIKeyboardHIDUsageKeyboardComma,
	UIKeyboardHIDUsageKeyboardPeriod,
	UIKeyboardHIDUsageKeyboardSlash,
	UIKeyboardHIDUsageKeyboardRightShift,
	UIKeyboardHIDUsageKeypadAsterisk,
	UIKeyboardHIDUsageKeyboardLeftAlt,
	UIKeyboardHIDUsageKeyboardSpacebar,
	UIKeyboardHIDUsageKeyboardCapsLock,
	UIKeyboardHIDUsageKeyboardF1,
	UIKeyboardHIDUsageKeyboardF2,
	UIKeyboardHIDUsageKeyboardF3,
	UIKeyboardHIDUsageKeyboardF4,
	UIKeyboardHIDUsageKeyboardF5,
	UIKeyboardHIDUsageKeyboardF6,
	UIKeyboardHIDUsageKeyboardF7,
	UIKeyboardHIDUsageKeyboardF8,
	UIKeyboardHIDUsageKeyboardF9,
	UIKeyboardHIDUsageKeyboardF10,
	UIKeyboardHIDUsageKeypadNumLock,
	UIKeyboardHIDUsageKeyboardScrollLock,
	UIKeyboardHIDUsageKeypad7,
	UIKeyboardHIDUsageKeypad8,
	UIKeyboardHIDUsageKeypad9,
	UIKeyboardHIDUsageKeypadHyphen,
	UIKeyboardHIDUsageKeypad4,
	UIKeyboardHIDUsageKeypad5,
	UIKeyboardHIDUsageKeypad6,
	UIKeyboardHIDUsageKeypadPlus,
	UIKeyboardHIDUsageKeypad1,
	UIKeyboardHIDUsageKeypad2,
	UIKeyboardHIDUsageKeypad3,
	UIKeyboardHIDUsageKeypad0,
	UIKeyboardHIDUsageKeypadPeriod,
	UIKeyboardHIDUsageKeyboardNonUSBackslash, // OEM_102
	UIKeyboardHIDUsageKeyboardF11,
	UIKeyboardHIDUsageKeyboardF12,
	UIKeyboardHIDUsageKeyboardF13,
	UIKeyboardHIDUsageKeyboardF14,
	UIKeyboardHIDUsageKeyboardF15,
	UIKeyboardHIDUsageKeyboardInternational2, // KANA
	UIKeyboardHIDUsageKeyboardInternational1, // ABNT_C1
	UIKeyboardHIDUsageKeyboardInternational4, // CONVERT
	UIKeyboardHIDUsageKeyboardInternational5, // NOCONVERT
	UIKeyboardHIDUsageKeyboardInternational3, // YEN
	0, // ABNT_C2
	UIKeyboardHIDUsageKeypadEqualSign,
	0, // PREVTRACK
	0, // AT
	0, // COLON
	0, // UNDERLINE
	0, // KANJI
	UIKeyboardHIDUsageKeyboardStop,
	0, // AX
	0, // UNLABELED
	0, // NEXTTRACK
	UIKeyboardHIDUsageKeypadEnter,
	UIKeyboardHIDUsageKeyboardRightControl,
	UIKeyboardHIDUsageKeyboardMute,
	0, // CALCULATOR
	0, // PLAYPAUSE
	0, // MEDIASTOP
	UIKeyboardHIDUsageKeyboardVolumeDown,
	UIKeyboardHIDUsageKeyboardVolumeUp,
	0, // WEBHOME
	UIKeyboardHIDUsageKeypadComma,
	UIKeyboardHIDUsageKeypadSlash,
	UIKeyboardHIDUsageKeyboardPrintScreen, // SYSRQ
	UIKeyboardHIDUsageKeyboardRightAlt,
	UIKeyboardHIDUsageKeyboardPause,
	UIKeyboardHIDUsageKeyboardHome,
	UIKeyboardHIDUsageKeyboardUpArrow,
	UIKeyboardHIDUsageKeyboardPageUp,
	UIKeyboardHIDUsageKeyboardLeftArrow,
	UIKeyboardHIDUsageKeyboardRightArrow,
	UIKeyboardHIDUsageKeyboardEnd,
	UIKeyboardHIDUsageKeyboardDownArrow,
	UIKeyboardHIDUsageKeyboardPageDown,
	UIKeyboardHIDUsageKeyboardInsert,
	UIKeyboardHIDUsageKeyboardDeleteForward,
	UIKeyboardHIDUsageKeyboardLeftGUI,
	UIKeyboardHIDUsageKeyboardRightGUI,
	UIKeyboardHIDUsageKeyboardApplication,
	UIKeyboardHIDUsageKeyboardPower,
	0, // SLEEP
	0, // WAKE
	0, // WEBSEARCH
	0, // WEBFAVORITES
	0, // WEBREFRESH
	0, // WEBSTOP
	0, // WEBFORWARD
	0, // WEBBACK
	0, // MYCOMPUTER
	0, // MAIL
	0  // MEDIASELECT
};

IOSKeyboard::IOSKeyboard(void)
{
}

IOSKeyboard::~IOSKeyboard(void)
{
	GCKeyboard* keyboard = GCKeyboard.coalescedKeyboard;
	if (keyboard && (__bridge const void*)keyboard == _attachedKeyboard) {
		keyboard.keyboardInput.keyChangedHandler = nil;
	}
}

void IOSKeyboard::onKeyChanged(int key, bool pressed)
{
	if (key < 0 || key >= kKeyCount) {
		return;
	}
	_held[(size_t)key] = pressed;
	// Latched so a press and release between two updates still reads as down
	if (pressed) {
		_keys.latchPress(key);
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
	for (int key = 0; key < kKeyCount; ++key) {
		_keys.set(key, _held[(size_t)key]);
	}
}

namespace {
bool axisIsDown(float value, float threshold)
{
	return threshold < 0.0f ? value <= threshold : value >= threshold;
}

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

IOSGamepad::IOSGamepad(TouchGamepadState* touch)
	: _touch(touch)
{
}

const IOSGamepad::Pad* IOSGamepad::_pad(int padIndex) const
{
	if (padIndex < 0 || padIndex >= _connected) {
		return nullptr;
	}
	return &_pads[(size_t)padIndex];
}

std::string IOSGamepad::getName(int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad ? pad->name : std::string();
}

bool IOSGamepad::buttonDown(Button button, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad && pad->buttons.down((int)button);
}

bool IOSGamepad::buttonPressed(Button button, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad && pad->buttons.pressed((int)button);
}

bool IOSGamepad::buttonReleased(Button button, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad && pad->buttons.released((int)button);
}

float IOSGamepad::getAxis(Axis axis, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad ? pad->axes[(size_t)axis] : 0.0f;
}

bool IOSGamepad::axisDown(Axis axis, float threshold, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad && axisIsDown(pad->axes[(size_t)axis], threshold);
}

bool IOSGamepad::axisPressed(Axis axis, float threshold, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad && ButtonEdge::pressed(axisIsDown(pad->axes[(size_t)axis], threshold),
		axisIsDown(pad->previousAxes[(size_t)axis], threshold));
}

bool IOSGamepad::axisReleased(Axis axis, float threshold, int padIndex) const
{
	const Pad* pad = _pad(padIndex);
	return pad && ButtonEdge::released(axisIsDown(pad->axes[(size_t)axis], threshold),
		axisIsDown(pad->previousAxes[(size_t)axis], threshold));
}

void IOSGamepad::update(void)
{
	struct Reading
	{
		std::string name;
		std::array<bool, TouchGamepadState::kButtonCount> down{};
		std::array<bool, TouchGamepadState::kButtonCount> tapped{};
		std::array<float, kAxisCount> axes{};
	};
	std::vector<Reading> readings;

	for (GCController* controller in GCController.controllers) {
		GCExtendedGamepad* gamepad = controller.extendedGamepad;
		if (!gamepad) {
			continue;
		}

		Reading& reading = readings.emplace_back();
		reading.name = controller.vendorName ? controller.vendorName.UTF8String : "Game controller";
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
			readings.emplace_back().name = "Touch controls";
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

	// Slots past the connected pads read as released, so a pad that comes back starts clean.
	if (_pads.size() < readings.size()) {
		_pads.resize(readings.size());
	}
	for (size_t i = 0; i < _pads.size(); ++i) {
		Pad& pad = _pads[i];
		const Reading* reading = (i < readings.size()) ? &readings[i] : nullptr;
		pad.connected = reading != nullptr;
		pad.name = reading ? reading->name : std::string();
		pad.buttons.beginFrame();
		for (int button = 0; button < TouchGamepadState::kButtonCount; ++button) {
			if (reading && reading->tapped[(size_t)button]) {
				pad.buttons.latchPress(button);
			}
			pad.buttons.set(button, reading && reading->down[(size_t)button]);
		}
		pad.previousAxes = pad.axes;
		if (reading) {
			pad.axes = reading->axes;
		}
		else {
			pad.axes.fill(0.0f);
		}
	}
	_connected = (int)readings.size();
}

IOSInput::IOSInput(TouchGamepadState* touch)
{
	_keyboard = new IOSKeyboard();
	_mouse = NULL; // touches go to the on-screen controls, not a pointer
	_gamepad = new IOSGamepad(touch);
}

IOSInput::~IOSInput(void)
{
	SAFE_DELETE(_keyboard);
	SAFE_DELETE(_gamepad);
}

void IOSInput::initialize(void)
{
	update();
}

void IOSInput::update(void)
{
	if (_keyboard) {
		_keyboard->update();
	}
	if (_gamepad) {
		_gamepad->update();
	}
}

void IOSInput::shutdown(void)
{
}
