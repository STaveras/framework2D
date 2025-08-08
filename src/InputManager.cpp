
#include "InputManager.h"
#include "EventSystem.h"
#include "InputEvent.h"
#include "Types.h"
#include "Controller.h"

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

Controller* InputManager::createController(void)
{
	Controller* controller = _controllers.create();
	controller->setPadNumber((int)(_controllers.size() - 1));
	controller->setInputInterface(_input);
	controller->setEventSystem(_eventSystem);
	return controller;
}

void InputManager::destroyController(Controller* controller)
{
	_controllers.destroy(controller);
}

void InputManager::update(float fTime)
{
	if (!_input)
		return;

	for (auto& controller : _controllers) {
		controller->update(fTime);
	}
}

void InputManager::shutdown(void)
{
	_controllers.clear();

	if (_eventSystem)
		_eventSystem = NULL;

	if (_input)
		_input = NULL;
}