// File: PauseState.cpp

#include "PauseState.h"

#include "../Engine2D.h"
#include "../InputMap.h"

PauseState::PauseState()
	: MenuState("PAUSE", 0xFFFFFFFF, { "RESUME", "QUIT" },
#if FRAMEWORK_IOS
		// The touch controls' pause button, or a controller's menu button
		"|| / MENU to Resume"
#else
		"ESC / OPTIONS to Resume"
#endif
	)
{
}

bool PauseState::onShortcut(const InputMap& input)
{
	// The same action that paused (Escape or controller Options) resumes.
	if (input.pressed("PAUSE")) {
		Engine2D::getGame()->pop();
		return true;
	}
	return false;
}

void PauseState::onChoose(int index)
{
	switch (index) {
		case OPTION_QUIT:
			Engine2D::quit();
			break;
		default: // OPTION_RESUME and anything else
			Engine2D::getGame()->pop();
			break;
	}
}
