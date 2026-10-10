// PlayerController.h
// A Controller that drives an Actor from an InputMap: it turns the map's
// named actions into the Actor's Intent, and can feed them to a state machine
// as transition conditions.

#pragma once

#include "Controller.h"

#include <string>
#include <vector>

class InputMap;
class StateMachine;

class PlayerController : public Controller
{
	struct Binding
	{
		int action;
		std::string name;
	};

	InputMap* _inputMap = nullptr;
	std::vector<Binding> _bindings;
	std::string _moveLeft, _moveRight, _moveUp, _moveDown;
	// Each action's state at the last sendActionConditions(), in map order
	std::vector<bool> _sentDown;

public:
	PlayerController(void) = default;
	explicit PlayerController(InputMap* inputMap) : _inputMap(inputMap) {}

	InputMap* getInputMap(void) const { return _inputMap; }
	void setInputMap(InputMap* inputMap);

	// Map an Intent action slot to an InputMap action name.
	void bindAction(int action, const char* actionName);
	// Map InputMap actions to the digital move axes (empty name = unbound).
	void bindMoveX(const char* negative, const char* positive);
	void bindMoveY(const char* negative, const char* positive);

	void updateIntent(const Actor& actor, float time, Intent& intent) override;

	// Send every action to target as "<ACTION>_DOWN" or "<ACTION>_UP", plus
	// "<ACTION>_PRESSED" or "<ACTION>_RELEASED" when it changed since the last
	// call, in map order. Call once per tick before the world updates.
	void sendActionConditions(StateMachine& target);
	// Treat every action as released at the last send, so a new target sees
	// actions that are already held as pressed.
	void resetActionConditions(void) { _sentDown.clear(); }
};
