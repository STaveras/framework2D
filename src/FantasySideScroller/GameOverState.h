// File: GameOverState.h
// The game-over menu: sits on top of PlayState so the world freezes (with the
// dead character visible) while it is shown. Pushed when the playable
// character dies. RETRY (or Y) pops it and PlayState respawns the hero; QUIT
// (or N) ends the game.

#pragma once

#include "MenuState.h"

class GameOverState : public MenuState
{
	enum Option { OPTION_RETRY = 0, OPTION_QUIT = 1 };

	bool _retryRequested = false;

protected:
	void onChoose(int index) override;
	bool onShortcut(const InputMap& input) override;

public:
	GameOverState(void);

	// True if the player chose to retry (consumed on next read).
	bool wasRetryRequested(void) const { return _retryRequested; }
	void consumeRetryRequest(void) { _retryRequested = false; }
};
