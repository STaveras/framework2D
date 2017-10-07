#include "Player.h"
#include "GameObject.h"

//void Player::Setup(void)
//{
//	if (_object)
//		_object->Initialize();
//}
//
//void Player::Update(float time)
//{
//	if (_object)
//		_object->Update(time);
//}
//
//void Player::Shutdown(void)
//{
//	if (_object)
//		_object->Reset();
//}
//#include "EventSystem.h"
//#include "InputEvent.h"
//
//void Player::_OnKeyPress(const Event& evt)
//{
//	InputEvent* pEvent = (InputEvent*)&evt;
//
//	if(pEvent->GetGamePad() == m_pGamePad)
//	{
//		// This is my gamepad.
//
//
//		// Where the rules for the player are to interface with the game.
//
//		//Console.
//		//this->_guts->
//	}
//}
//
//void Player::_OnKeyRelease(const Event& evt)
//{
//	InputEvent* pEvent = (InputEvent*)&evt;
//
//	if(pEvent->GetGamePad() == m_pGamePad)
//	{
//
//	}
//}
//
//void Player::Initialize(EventSystem* pEventSystem)
//{
//	m_pEventSystem = pEventSystem;
//
//	m_pEventSystem->RegisterCallback<Player>("EVT_KEYPRESSED", this, &Player::_OnKeyPress);
//	m_pEventSystem->RegisterCallback<Player>("EVT_KEYRELEASED", this, &Player::_OnKeyRelease);
//}
//
//void Player::Update(float fTime)
//{
//
//}
//
//void Player::Shutdown(void)
//{
//	m_pEventSystem->UnregisterAll<Player>(this, &Player::_OnKeyRelease);
//	m_pEventSystem->UnregisterAll<Player>(this, &Player::_OnKeyPress);
//}