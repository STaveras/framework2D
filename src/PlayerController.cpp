#include "PlayerController.h"

#include "InputMap.h"

namespace {
bool isActive(InputMap* inputMap, const std::string& name)
{
	if (name.empty()) {
		return false;
	}
	Action* action = inputMap->getAction(name);
	return action && action->isActive();
}

float axis(InputMap* inputMap, const std::string& negative, const std::string& positive)
{
	return (isActive(inputMap, positive) ? 1.0f : 0.0f) - (isActive(inputMap, negative) ? 1.0f : 0.0f);
}
}

void PlayerController::bindAction(int action, const char* actionName)
{
	if (!Intent::validAction(action) || !actionName) {
		return;
	}
	_bindings.push_back({ action, actionName });
}

void PlayerController::bindMoveX(const char* negative, const char* positive)
{
	_moveLeft = negative ? negative : "";
	_moveRight = positive ? positive : "";
}

void PlayerController::bindMoveY(const char* negative, const char* positive)
{
	_moveUp = negative ? negative : "";
	_moveDown = positive ? positive : "";
}

void PlayerController::updateIntent(const Actor& actor, float time, Intent& intent)
{
	if (!_inputMap) {
		intent.setHeld(0);
		intent.move = vector2(0.0f, 0.0f);
		return;
	}

	uint32_t held = 0;
	for (const Binding& binding : _bindings) {
		if (isActive(_inputMap, binding.name)) {
			held |= Intent::bit(binding.action);
		}
	}
	intent.setHeld(held);
	intent.move = vector2(axis(_inputMap, _moveLeft, _moveRight), axis(_inputMap, _moveUp, _moveDown));
}
