// File: InputManager.h
// Owns a game's InputMaps (one per player) and samples them once per tick.
// The Game holds one; see Game::update().

#pragma once

#include "InputMap.h"

#include <deque>

class IInput;

class InputManager
{
	IInput* _input = nullptr;
	// A deque keeps the references createInputMap() returns valid.
	std::deque<InputMap> _inputMaps;

public:
	void initialize(IInput* input);

	// A new map reading the given gamepad (0 is the first connected pad).
	InputMap& createInputMap(int padIndex = 0);

	void update(float time);
	void shutdown(void);
};
