#include "InputManager.h"

void InputManager::initialize(IInput* input)
{
	_input = input;
	for (InputMap& map : _inputMaps) {
		map.setInput(input);
	}
}

InputMap& InputManager::createInputMap(int padIndex)
{
	InputMap& map = _inputMaps.emplace_back(_input);
	map.setPadIndex(padIndex);
	return map;
}

void InputManager::update(float time)
{
	for (InputMap& map : _inputMaps) {
		map.update(time);
	}
}

void InputManager::shutdown(void)
{
	_inputMaps.clear();
	_input = nullptr;
}
