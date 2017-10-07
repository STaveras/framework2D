
#include "InputManager.h"
#include "EventSystem.h"
#include "InputEvent.h"
#include "Types.h"
#include "VirtualGamePad.h"

InputManager::InputManager(void):
m_pEventSystem(NULL),
	m_pInput(NULL)
{}

InputManager::~InputManager(void){}

VirtualGamePad* InputManager::CreateGamePad(void)
{
	VirtualGamePad* pGamePad = m_GamePads.Create();	
	pGamePad->SetPadNumber(m_GamePads.Size() - 1);

	return pGamePad;
}

void InputManager::Update(float fTime)
{
	m_fElapsed += fTime;

	for(Factory<VirtualGamePad>::const_factory_iterator vpad_itr = m_GamePads.Begin(); vpad_itr != m_GamePads.End(); vpad_itr++)
	{
		std::list<VirtualButton>::iterator itr = (*vpad_itr)->GetButtons().begin();

		for(; itr != (*vpad_itr)->GetButtons().end(); itr++)
		{
			std::list<KEYBOARD_KEYS>::const_iterator citr = itr->GetAssignments().begin();

			for(; citr != itr->GetAssignments().end(); citr++)
			{
				if(m_pInput->GetKeyboard()->KeyPressed((*citr)))
				{
					if(m_pEventSystem)
						m_pEventSystem->SendEvent<InputEvent>(InputEvent("EVT_KEYPRESSED", this, (*vpad_itr), m_fElapsed, itr->GetButtonID()));

					break;
				}
				else if(m_pInput->GetKeyboard()->KeyReleased((*citr)))
				{
					if(m_pEventSystem)
						m_pEventSystem->SendEvent<InputEvent>(InputEvent("EVT_KEYRELEASED", this, (*vpad_itr), m_fElapsed, itr->GetButtonID()));

					break;
				}
			}
		}
	}
}

void InputManager::Shutdown(void)
{
	if (m_pEventSystem)
		m_pEventSystem = NULL;

	if (m_pInput)
		m_pInput = NULL;
}