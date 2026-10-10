// File: PauseState.h
// The pause menu: sits on top of PlayState so the game freezes while the
// world stays visible. Pushed when PAUSE is pressed; PAUSE or RESUME resumes,
// QUIT ends the game.

#pragma once

#include "MenuState.h"

class PauseState : public MenuState
{
	enum Option { OPTION_RESUME = 0, OPTION_QUIT = 1 };

protected:
	void onChoose(int index) override;
	bool onShortcut(const InputMap& input) override;

public:
	PauseState(void);
};
