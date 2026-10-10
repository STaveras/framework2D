// FantasySideScroller
// An example game using this framework
////////////////////////////////////////

#pragma once

#include "../Game.h"

#include <memory>

class InputMap;
class PlayState;

class FantasySideScroller : public Game
{
	std::unique_ptr<PlayState> _playState;
	InputMap* _inputMap = nullptr; // owned by _inputManager

	void _bindInputs(InputMap& inputMap);

public:
	FantasySideScroller(void);
	~FantasySideScroller(void);

	void begin(void) override;
	void end(void) override;

	// The player's actions: the hero's, plus the UP / DOWN / CONFIRM / PAUSE
	// actions the pause and game-over menus share. Valid after begin().
	InputMap& getInputMap(void) const { return *_inputMap; }
};

extern FantasySideScroller game;
