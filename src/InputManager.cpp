
#include "InputManager.h"
#include "EventSystem.h"
#include "InputEvent.h"
#include "Types.h"
#include "Controller.h"

InputManager::InputManager(void):
m_pEventSystem(NULL),
	m_pInput(NULL)
{}

InputManager::~InputManager(void){}

Controller* InputManager::CreateController(void)
{
	Controller* pGamePad = m_Controllers.Create();	
	pGamePad->setPadNumber((int)(m_Controllers.Size() - 1));

	return pGamePad;
}

void InputManager::update(float fTime)
{
	m_fElapsed += fTime;

	if (!m_pInput)
		return;

	for(Factory<Controller>::const_factory_iterator vpad_itr = m_Controllers.Begin(); vpad_itr != m_Controllers.End(); vpad_itr++)
	{
		std::list<Action>::iterator itr = (*vpad_itr)->getActions().begin();

		for(; itr != (*vpad_itr)->getActions().end(); itr++)
		{
			std::list<KEYBOARD_KEYS>::const_iterator citr = itr->getAssignments().begin();

			for(; citr != itr->getAssignments().end(); citr++)
			{
            if (m_pInput->GetKeyboard()) {

               if (m_pInput->GetKeyboard()->KeyPressed((*citr)))
               {
                  if (m_pEventSystem)
                     m_pEventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYPRESS, this, (*vpad_itr), m_fElapsed, itr->getActionName()));

                  break;
               }
               else if (m_pInput->GetKeyboard()->KeyReleased((*citr)))
               {
                  if (m_pEventSystem)
                     m_pEventSystem->sendEvent<InputEvent>(InputEvent(EVT_KEYRELEASE, this, (*vpad_itr), m_fElapsed, itr->getActionName()));

                  break;
               }
            }
			}
		}
	}
}

void InputManager::shutdown(void)
{
	if (m_pEventSystem)
		m_pEventSystem = NULL;

	if (m_pInput)
		m_pInput = NULL;
}