// TouchControls.mm
#if !__has_feature(objc_arc)
#error "TouchControls.mm must be compiled with -fobjc-arc"
#endif

#import "TouchControls.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace {
using Button = Gamepad::Button;

constexpr CGFloat kEdgeMargin = 20.0;
constexpr CGFloat kStickRadius = 62.0;
constexpr CGFloat kKnobRadius = 26.0;
constexpr CGFloat kButtonSlop = 14.0;     // extra hit radius around each button
constexpr float kDeadZone = 0.2f;         // stick travel ignored around the centre
constexpr float kDpadHorizontal = 0.4f;   // stick x past which D-pad left/right are held
constexpr float kDpadVertical = 0.65f;    // stick y past which D-pad up/down are held
constexpr int kNoButton = -1;

struct ActionButton
{
	Button button;
	NSString* label;
	NSString* symbol; // SF Symbol shown instead of the label, if set
	CGFloat radius;
	bool toggle;

	CGPoint center = CGPointZero;
	bool toggledOn = false;
	UIView* view = nil;
};

CGFloat distance(CGPoint a, CGPoint b)
{
	return std::hypot(a.x - b.x, a.y - b.y);
}

UIColor* idleFill(void) { return [UIColor colorWithWhite:0.0 alpha:0.28]; }
UIColor* pressedFill(void) { return [UIColor colorWithWhite:1.0 alpha:0.4]; }

UIView* makeCircle(CGFloat radius)
{
	UIView* circle = [[UIView alloc] initWithFrame:CGRectMake(0.0, 0.0, radius * 2.0, radius * 2.0)];
	circle.userInteractionEnabled = NO;
	circle.backgroundColor = idleFill();
	circle.layer.cornerRadius = radius;
	circle.layer.borderWidth = 2.0;
	circle.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.6].CGColor;
	return circle;
}
}

@implementation FWTouchControlsView {
	TouchGamepadState* _state;
	std::vector<ActionButton> _buttons;
	// Every touch on the controls that is not the stick, and the button under it.
	std::unordered_map<const void*, int> _buttonTouches;
	UITouch* _stickTouch;
	CGPoint _stickHome;
	CGPoint _stickCenter;
	CGPoint _stickVector; // knob offset in stick radii, length <= 1
	UIView* _stickBase;
	UIView* _stickKnob;
	UIImpactFeedbackGenerator* _haptics;
}

- (instancetype)initWithState:(TouchGamepadState*)state
{
	self = [super initWithFrame:CGRectZero];
	if (!self) {
		return nil;
	}

	_state = state;
	_shown = NO;
	self.alpha = 0.0;
	self.userInteractionEnabled = NO;
	self.multipleTouchEnabled = YES;
	self.backgroundColor = UIColor.clearColor;
	_haptics = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];

	_stickBase = makeCircle(kStickRadius);
	_stickKnob = makeCircle(kKnobRadius);
	_stickKnob.backgroundColor = [UIColor colorWithWhite:1.0 alpha:0.35];
	[self addSubview:_stickBase];
	[self addSubview:_stickKnob];

	_buttons = {
		{ Button::A, @"JUMP", nil, 40.0, false },
		{ Button::X, @"ATTACK", nil, 34.0, false },
		{ Button::LeftThumb, @"USE", nil, 26.0, false },
		{ Button::LeftBumper, @"RUN", nil, 26.0, true },
		{ Button::Start, nil, @"pause.fill", 18.0, false },
	};
	for (ActionButton& button : _buttons) {
		button.view = makeCircle(button.radius);
		if (button.symbol) {
			UIImageView* icon = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:button.symbol]];
			icon.tintColor = [UIColor colorWithWhite:1.0 alpha:0.9];
			icon.contentMode = UIViewContentModeScaleAspectFit;
			icon.frame = CGRectInset(button.view.bounds, button.radius * 0.5, button.radius * 0.5);
			[button.view addSubview:icon];
		}
		else {
			UILabel* label = [[UILabel alloc] initWithFrame:button.view.bounds];
			label.text = button.label;
			label.textAlignment = NSTextAlignmentCenter;
			label.textColor = [UIColor colorWithWhite:1.0 alpha:0.9];
			label.font = [UIFont systemFontOfSize:(button.radius >= 34.0 ? 12.0 : 10.0) weight:UIFontWeightHeavy];
			label.adjustsFontSizeToFitWidth = YES;
			[button.view addSubview:label];
		}
		[self addSubview:button.view];
	}
	return self;
}

- (void)layoutSubviews
{
	[super layoutSubviews];

	// Keep clear of the sensor housing and the home indicator.
	const UIEdgeInsets safe = self.safeAreaInsets;
	const CGRect bounds = self.bounds;
	const CGFloat left = CGRectGetMinX(bounds) + std::max(safe.left, kEdgeMargin) + 8.0;
	const CGFloat right = CGRectGetMaxX(bounds) - std::max(safe.right, kEdgeMargin) - 8.0;
	// Further from the top: iOS holds back touches along that edge while it
	// checks for a system swipe, which can swallow quick taps.
	const CGFloat top = CGRectGetMinY(bounds) + std::max(safe.top, kEdgeMargin) + 20.0;
	const CGFloat bottom = CGRectGetMaxY(bounds) - std::max(safe.bottom, kEdgeMargin) - 4.0;

	_stickHome = CGPointMake(left + kStickRadius + 12.0, bottom - kStickRadius - 8.0);
	if (!_stickTouch) {
		_stickCenter = _stickHome;
	}

	const CGPoint centers[] = {
		CGPointMake(right - 44.0, bottom - 52.0),        // JUMP
		CGPointMake(right - 136.0, bottom - 30.0),       // ATTACK
		CGPointMake(right - 30.0, bottom - 136.0),       // USE
		CGPointMake(right - 140.0, bottom - 110.0),      // RUN
		CGPointMake(right - 18.0, top + 18.0),           // PAUSE
	};
	for (size_t i = 0; i < _buttons.size(); ++i) {
		_buttons[i].center = centers[i];
		_buttons[i].view.center = centers[i];
	}
	[self updateStickViews];
}

- (void)setShown:(BOOL)shown animated:(BOOL)animated
{
	if (shown == _shown) {
		return;
	}
	_shown = shown;

	[self releaseAll];
	_state->active = shown;
	self.userInteractionEnabled = shown;
	const CGFloat alpha = shown ? 1.0 : 0.0;
	if (animated) {
		[UIView animateWithDuration:0.25 animations:^{ self.alpha = alpha; }];
	}
	else {
		self.alpha = alpha;
	}
}

- (void)releaseAll
{
	_buttonTouches.clear();
	_stickTouch = nil;
	_stickVector = CGPointZero;
	_stickCenter = _stickHome;
	for (ActionButton& button : _buttons) {
		button.toggledOn = false;
	}
	_state->release();
	[self publish];
}

// Index of the button under point, or kNoButton.
- (int)buttonAt:(CGPoint)point includeToggles:(BOOL)includeToggles
{
	int best = kNoButton;
	CGFloat bestDistance = INFINITY;
	for (size_t i = 0; i < _buttons.size(); ++i) {
		const ActionButton& button = _buttons[i];
		if (button.toggle && !includeToggles) {
			continue;
		}
		const CGFloat d = distance(point, button.center);
		if (d <= button.radius + kButtonSlop && d < bestDistance) {
			best = (int)i;
			bestDistance = d;
		}
	}
	return best;
}

- (void)moveStickTo:(CGPoint)point
{
	CGFloat dx = (point.x - _stickCenter.x) / kStickRadius;
	CGFloat dy = (point.y - _stickCenter.y) / kStickRadius;
	const CGFloat length = std::hypot(dx, dy);
	if (length > 1.0) {
		dx /= length;
		dy /= length;
	}
	_stickVector = CGPointMake(dx, dy);
}

- (void)updateStickViews
{
	_stickBase.center = _stickCenter;
	_stickKnob.center = CGPointMake(_stickCenter.x + _stickVector.x * kStickRadius,
		_stickCenter.y + _stickVector.y * kStickRadius);
}

// Write the controls' state to the virtual gamepad and update the visuals.
- (void)publish
{
	std::array<bool, TouchGamepadState::kButtonCount> down{};

	float x = (float)_stickVector.x;
	float y = (float)_stickVector.y;
	if (std::hypot(x, y) < kDeadZone) {
		x = y = 0.0f;
	}
	_state->leftX = x;
	_state->leftY = y;
	down[(size_t)Button::DpadLeft] = x < -kDpadHorizontal;
	down[(size_t)Button::DpadRight] = x > kDpadHorizontal;
	down[(size_t)Button::DpadUp] = y < -kDpadVertical;
	down[(size_t)Button::DpadDown] = y > kDpadVertical;

	std::vector<bool> lit(_buttons.size(), false);
	for (const auto& touch : _buttonTouches) {
		if (touch.second != kNoButton) {
			lit[(size_t)touch.second] = true;
		}
	}
	for (size_t i = 0; i < _buttons.size(); ++i) {
		const ActionButton& button = _buttons[i];
		if (button.toggle) {
			lit[i] = button.toggledOn;
		}
		if (lit[i]) {
			down[(size_t)button.button] = true;
		}
		button.view.backgroundColor = lit[i] ? pressedFill() : idleFill();
	}

	if (_state->active) {
		for (size_t i = 0; i < down.size(); ++i) {
			if (down[i] && !_state->down[i]) {
				_state->tapped[i] = true;
			}
		}
		_state->down = down;
	}
	[self updateStickViews];
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	for (UITouch* touch in touches) {
		const CGPoint point = [touch locationInView:self];
		const int index = [self buttonAt:point includeToggles:YES];
		if (index != kNoButton) {
			ActionButton& button = _buttons[(size_t)index];
			if (button.toggle) {
				button.toggledOn = !button.toggledOn;
				_buttonTouches[(__bridge const void*)touch] = kNoButton;
			}
			else {
				_buttonTouches[(__bridge const void*)touch] = index;
			}
			[_haptics impactOccurred];
		}
		else if (!_stickTouch && point.x < CGRectGetMidX(self.bounds)) {
			// Re-centre the stick under the thumb unless it landed on the stick.
			_stickTouch = touch;
			_stickCenter = (distance(point, _stickHome) <= kStickRadius * 1.3) ? _stickHome : point;
			[self moveStickTo:point];
		}
		else {
			// Not on a button yet, but sliding onto one presses it.
			_buttonTouches[(__bridge const void*)touch] = kNoButton;
		}
	}
	[self publish];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	for (UITouch* touch in touches) {
		const CGPoint point = [touch locationInView:self];
		if (touch == _stickTouch) {
			[self moveStickTo:point];
			continue;
		}

		auto bound = _buttonTouches.find((__bridge const void*)touch);
		if (bound == _buttonTouches.end()) {
			continue;
		}
		const int index = [self buttonAt:point includeToggles:NO];
		if (index != bound->second) {
			bound->second = index;
			if (index != kNoButton) {
				[_haptics impactOccurred];
			}
		}
	}
	[self publish];
}

- (void)endTouches:(NSSet<UITouch*>*)touches
{
	for (UITouch* touch in touches) {
		if (touch == _stickTouch) {
			_stickTouch = nil;
			_stickVector = CGPointZero;
			_stickCenter = _stickHome;
			// Running stops with the player.
			for (ActionButton& button : _buttons) {
				button.toggledOn = false;
			}
			continue;
		}
		_buttonTouches.erase((__bridge const void*)touch);
	}
	[self publish];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[self endTouches:touches];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
	[self endTouches:touches];
}

@end
