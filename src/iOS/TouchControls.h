// TouchControls.h
// On-screen controls for playing without a game controller, drawn by UIKit
// over the game view and fed to the engine as a virtual gamepad
// (TouchGamepadState, read by IOSGamepad):
//   left:  a stick (LeftX/LeftY, plus the D-pad past the edge zones). Touch
//          anywhere on the left half; outside the stick it re-centres there.
//   right: JUMP (A), ATTACK (X), USE (left thumb), RUN (left bumper, a toggle
//          that turns off when the stick is let go), and PAUSE (Start).
// Fingers can slide between the action buttons.

#pragma once

#import <UIKit/UIKit.h>

#include "Input.h"

@interface FWTouchControlsView : UIView

// Must outlive the view.
- (instancetype)initWithState:(TouchGamepadState*)state;

// Hidden controls release everything, ignore touches and stop feeding the pad.
@property (nonatomic, readonly) BOOL shown;
- (void)setShown:(BOOL)shown animated:(BOOL)animated;

@end
