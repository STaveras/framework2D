// PlayerController.h
// A Controller that turns an InputMap's named actions into an Intent.

#pragma once

#include "Controller.h"

#include <string>
#include <vector>

class InputMap;

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

public:
	PlayerController(void) = default;
	explicit PlayerController(InputMap* inputMap) : _inputMap(inputMap) {}

	InputMap* getInputMap(void) const { return _inputMap; }
	void setInputMap(InputMap* inputMap) { _inputMap = inputMap; }

	// Map an Intent action slot to an InputMap action name.
	void bindAction(int action, const char* actionName);
	// Map InputMap actions to the digital move axes (empty name = unbound).
	void bindMoveX(const char* negative, const char* positive);
	void bindMoveY(const char* negative, const char* positive);

	void updateIntent(const Actor& actor, float time, Intent& intent) override;
};
