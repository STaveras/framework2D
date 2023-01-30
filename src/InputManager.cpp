
#include "InputManager.h"
#include "EventSystem.h"
#include "InputEvent.h"
#include "Types.h"
#include "Controller.h"

InputManager::InputManager(void):
	_eventSystem(NULL),
	_input(NULL)
{}

InputManager::~InputManager(void){}

Controller* InputManager::createController(void)
{
	Controller* pGamePad = m_Controllers.create();	
	pGamePad->setPadNumber((int)(m_Controllers.size() - 1));

	return pGamePad;
}

void InputManager::update(float fTime)
{
	_elapsedTime += fTime;

	if (!_input)
		return;

	for(Factory<Controller>::const_factory_iterator vpad_itr = m_Controllers.begin(); vpad_itr != m_Controllers.end(); vpad_itr++)
	{
		std::list<Action>::iterator itr = (*vpad_itr)->getActions().begin();

		for(; itr != (*vpad_itr)->getActions().end(); itr++)
		{
			std::list<Keyboard::KEY>::const_iterator citr = itr->getAssignments().begin();

			for(; citr != itr->getAssignments().end(); citr++)
			{
            if (_input->getKeyboard()) {

               if (_input->getKeyboard()->keyPressed((*citr)))
               {
                  if (_eventSystem)
                     _eventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYPRESS, this, (*vpad_itr), _elapsedTime, itr->getActionName()));

                  break;
               }
               else if (_input->getKeyboard()->keyReleased((*citr)))
               {
                  if (_eventSystem)
                     _eventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYRELEASE, this, (*vpad_itr), _elapsedTime, itr->getActionName()));

                  break;
               }
            }
			}
		}
	}
}

void InputManager::shutdown(void)
{
	if (_eventSystem)
		_eventSystem = NULL;

	if (_input)
		_input = NULL;
}