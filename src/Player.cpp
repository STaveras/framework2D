#include "Player.h"
#include "InputEvent.h"
#include "GameObject.h"
#include "Engine2D.h"

void Player::_OnKeyPress(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->getController() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->getActionName() + "_PRESSED").c_str(), e.getSender());
   }
}

void Player::_OnKeyRelease(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->getController() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->getActionName() + "_RELEASED").c_str(), e.getSender());
   }
}

void Player::start(void)
{
	 Engine2D::getInstance()->getEventSystem()->registerCallback<Player>(EVT_KEYPRESS, this, &Player::_OnKeyPress);
	 Engine2D::getInstance()->getEventSystem()->registerCallback<Player>(EVT_KEYRELEASE, this, &Player::_OnKeyRelease);
}

void Player::update(float time)
{ 
   Controller* controller = this->getController();	

   for (Action action : controller->getActions()) {
      for (auto key : action.getAssignments()) {
         if (Engine2D::getInput()->getKeyboard()->KeyDown(key)) {
            if (this->getGameObject()) {
               this->getGameObject()->sendInput((action.getActionName() + "_DOWN").c_str(), this);
            }
         }
         else if (Engine2D::getInput()->getKeyboard()->KeyUp(key)) {
            if (this->getGameObject()) {
               this->getGameObject()->sendInput((action.getActionName() + "_UP").c_str(), this);
            }
         }
      }
   }
}

void Player::finish(void)
{
	 Engine2D::getInstance()->getEventSystem()->unregister<Player>(EVT_KEYRELEASE, this, &Player::_OnKeyRelease);
	 Engine2D::getInstance()->getEventSystem()->unregister<Player>(EVT_KEYPRESS, this, &Player::_OnKeyPress);
}