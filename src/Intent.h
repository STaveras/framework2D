// Intent.h
// What an Actor's Controller wants it to do this tick. Fixed-size so it is
// cheap to produce every frame from player input or AI alike. Action slots
// are small integers the game defines (e.g. an enum of its actions).

#pragma once

#include "Types.h"

#include <cstdint>

struct Intent
{
	static constexpr int kMaxActions = 32;

	// Movement in [-1, 1] per axis; +x is right, +y is down (screen space).
	vector2 move = vector2(0.0f, 0.0f);

	uint32_t held = 0;     // Actions active this tick
	uint32_t pressed = 0;  // Actions that became active this tick
	uint32_t released = 0; // Actions that stopped being active this tick

	static bool validAction(int action) { return action >= 0 && action < kMaxActions; }
	static uint32_t bit(int action) { return validAction(action) ? (1u << action) : 0u; }

	bool isHeld(int action) const { return (held & bit(action)) != 0; }
	bool wasPressed(int action) const { return (pressed & bit(action)) != 0; }
	bool wasReleased(int action) const { return (released & bit(action)) != 0; }

	// Set this tick's held actions; pressed/released follow from the
	// previously held set.
	void setHeld(uint32_t nowHeld)
	{
		pressed = nowHeld & ~held;
		released = held & ~nowHeld;
		held = nowHeld;
	}

	void clear(void) { *this = Intent(); }
};
