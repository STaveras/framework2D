#include "PlayerController.h"

#include "ButtonState.h"
#include "InputMap.h"
#include "StateMachine.h"

namespace {
bool isDown(const InputMap* inputMap, const std::string& name)
{
	return !name.empty() && inputMap->down(name);
}

float axis(const InputMap* inputMap, const std::string& negative, const std::string& positive)
{
	return (isDown(inputMap, positive) ? 1.0f : 0.0f) - (isDown(inputMap, negative) ? 1.0f : 0.0f);
}
}

void PlayerController::setInputMap(InputMap* inputMap)
{
	_inputMap = inputMap;
	resetActionConditions();
}

void PlayerController::bindAction(int action, const char* actionName)
{
	if (!Intent::validAction(action) || !actionName) {
		return;
	}
	for (const Binding& binding : _bindings) {
		if (binding.action == action && binding.name == actionName) {
			return;
		}
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
		if (isDown(_inputMap, binding.name)) {
			held |= Intent::bit(binding.action);
		}
	}
	intent.setHeld(held);
	intent.move = vector2(axis(_inputMap, _moveLeft, _moveRight), axis(_inputMap, _moveUp, _moveDown));
}

void PlayerController::sendActionConditions(StateMachine& target)
{
	if (!_inputMap) {
		return;
	}

	const auto& actions = _inputMap->getActions();
	_sentDown.resize(actions.size(), false);
	size_t index = 0;
	for (const Action& action : actions) {
		const bool down = action.down();
		const bool wasDown = _sentDown[index];
		_sentDown[index++] = down;

		const std::string& name = action.getName();
		target.sendInput((name + (down ? "_DOWN" : "_UP")).c_str(), _inputMap);
		if (ButtonEdge::pressed(down, wasDown)) {
			target.sendInput((name + "_PRESSED").c_str(), _inputMap);
		}
		else if (ButtonEdge::released(down, wasDown)) {
			target.sendInput((name + "_RELEASED").c_str(), _inputMap);
		}
	}
}
