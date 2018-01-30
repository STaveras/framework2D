#include "Player.h"
#include "Engine2D.h"
#include "InputEvent.h"

void Player::_OnKeyPress(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->GetGamePad() == this->_pad) {
      this->_object->SendInput(((std::string)((InputEvent*)&e)->GetButtonID() + "_PRESSED").c_str(), e.GetSender());
   }
}

void Player::_OnKeyRelease(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->GetGamePad() == this->_pad) {
      this->_object->SendInput(((std::string)((InputEvent*)&e)->GetButtonID() + "_RELEASED").c_str(), e.GetSender());
   }
}

void Player::Setup(void)
{
   Engine2D::GetInstance()->GetEventSystem()->RegisterCallback<Player>("EVT_KEYPRESSED", this, &Player::_OnKeyPress);
   Engine2D::GetInstance()->GetEventSystem()->RegisterCallback<Player>("EVT_KEYRELEASED", this, &Player::_OnKeyRelease);
}

void Player::Shutdown(void)
{
   Engine2D::GetInstance()->GetEventSystem()->Unregister<Player>("EVT_KEYRELEASED", this, &Player::_OnKeyRelease);
   Engine2D::GetInstance()->GetEventSystem()->Unregister<Player>("EVT_KEYPRESSED", this, &Player::_OnKeyPress);
}