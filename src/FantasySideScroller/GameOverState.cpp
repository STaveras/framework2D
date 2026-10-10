// File: GameOverState.cpp

#include "GameOverState.h"

#include "../Engine2D.h"
#include "../IInput.h"

GameOverState::GameOverState()
	: MenuState("GAME OVER", 0xFFFF5555, { "RETRY", "QUIT" },
#if FRAMEWORK_IOS
		// The touch controls' JUMP button, or a controller's A button
		"JUMP / A to Select"
#else
		"Y: RETRY   N: QUIT"
#endif
	)
{
}

bool GameOverState::onShortcut(const InputMap& input)
{
	(void)input;
	Keyboard* keyboard = Engine2D::getInput()->getKeyboard();
	if (!keyboard) {
		return false;
	}

	if (keyboard->pressed(Key::Y)) {
		onChoose(OPTION_RETRY);
		return true;
	}
	if (keyboard->pressed(Key::N)) {
		onChoose(OPTION_QUIT);
		return true;
	}
	// Space chooses the selected option, like CONFIRM.
	if (keyboard->pressed(Key::Space)) {
		onChoose(getSelection());
		return true;
	}
	return false;
}

void GameOverState::onChoose(int index)
{
	switch (index) {
		case OPTION_QUIT:
			Engine2D::quit();
			break;
		default: // OPTION_RETRY and anything else
			// Flag the request and pop; PlayState respawns on the way back up.
			_retryRequested = true;
			Engine2D::getGame()->pop();
			break;
	}
}
