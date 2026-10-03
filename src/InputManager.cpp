
#include "InputManager.h"
#include "EventSystem.h"
#include "InputEvent.h"
#include "Types.h"
#include "InputMap.h"

InputManager::InputManager(void):
	_eventSystem(NULL),
	_input(NULL)
{}

InputManager::~InputManager(void) {
	shutdown(); // Just to be nice
}

void InputManager::initialize(EventSystem* eventSystem, IInput* inputInterface)
{
	_eventSystem = eventSystem; 
	_input		 = inputInterface;
}

InputMap* InputManager::createInputMap(void)
{
	InputMap* controller = _inputMaps.create();
	controller->setPadNumber((int)(_inputMaps.size() - 1));
	controller->setInputInterface(_input);
	controller->setEventSystem(_eventSystem);
	return controller;
}

void InputManager::destroyInputMap(InputMap* controller)
{
	_inputMaps.destroy(controller);
}

void InputManager::update(float fTime)
{
	if (!_input)
		return;

	for (auto& controller : _inputMaps) {
		controller->update(fTime);
	}
}

void InputManager::shutdown(void)
{
	_inputMaps.clear();

	if (_eventSystem)
		_eventSystem = NULL;

	if (_input)
		_input = NULL;
}
