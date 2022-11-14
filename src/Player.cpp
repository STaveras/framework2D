#include "Player.h"
#include "InputEvent.h"
#include "GameObject.h"
#include "Engine2D.h"

void Player::_OnKeyPress(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->GetGamePad() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->setActionName() + "_PRESSED").c_str(), e.getSender());
   }
}

void Player::_OnKeyRelease(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->GetGamePad() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->setActionName() + "_RELEASED").c_str(), e.getSender());
   }
}

void Player::start(void)
{
	 Engine2D::getInstance()->getEventSystem()->registerCallback<Player>(EVT_KEYPRESS, this, &Player::_OnKeyPress);
	 Engine2D::getInstance()->getEventSystem()->registerCallback<Player>(EVT_KEYRELEASE, this, &Player::_OnKeyRelease);
}

void Player::update(float time)
{ 
	//if (Engine2D::getInput()->GetKeyboard()->KeyDown())
	//	_heading += vector2(-1, 0);
}

void Player::shutdown(void)
{
	 Engine2D::getInstance()->getEventSystem()->unregister<Player>(EVT_KEYRELEASE, this, &Player::_OnKeyRelease);
	 Engine2D::getInstance()->getEventSystem()->unregister<Player>(EVT_KEYPRESS, this, &Player::_OnKeyPress);
}