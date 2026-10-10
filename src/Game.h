// Game.h
// Is the "Application" built with this framework

#pragma once

#include "ProgramStack.h"
#include "InputManager.h"
#include "Engine2D.h"

class Game : public ProgramStack
{
	friend class Engine2D;

protected:
	// The game's input maps, sampled once per tick before the top state runs.
	InputManager _inputManager;

public:
	InputManager& getInputManager(void) { return _inputManager; }

	// Subclasses call these from their own begin() and end().
	virtual void begin(void) = 0;
	virtual void update(class Timer* timer);
	virtual void end(void) = 0;
};
