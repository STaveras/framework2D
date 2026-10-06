// Controller.h
// Drives an Actor by producing its Intent each tick. A PlayerController reads
// an InputMap; an AIController subclass decides in game code. The same Actor
// can be driven by either.

#pragma once

#include "Intent.h"

class Actor;

class Controller
{
public:
	virtual ~Controller(void) = default;

	// Called by the possessed Actor at the start of its update. intent still
	// holds the previous tick's values so edges can be derived from it.
	virtual void updateIntent(const Actor& actor, float time, Intent& intent) = 0;
};

// TODO: Base for computer-driven controllers; game code derives from this.
class AIController : public Controller
{
};
