// FantasySideScroller
// An example game using this framework
////////////////////////////////////////

#pragma once

#include "../Game.h"

class InputMap;
class PlayState; // Forward declaration

// Game should hold all the managers?
class FantasySideScroller : public Game
{
	PlayState* _playState;

public:
	FantasySideScroller()
		: _playState(nullptr) {
	}
	~FantasySideScroller() { }

	void begin(void);
	void end(void);

	// Adds the game's keyboard and gamepad bindings to inputMap: the hero's
	// actions plus the UP / DOWN / CONFIRM / PAUSE actions the pause and
	// game-over menus share.
	void bindInputs(InputMap* inputMap) const;
};

extern FantasySideScroller game;