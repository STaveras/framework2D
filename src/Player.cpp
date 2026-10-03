#include "Player.h"
#include "InputEvent.h"
#include "GameObject.h"
#include "Engine2D.h"

void Player::onButtonPressed(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->getInputMap() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->getActionName() + "_PRESSED").c_str(), e.getSender());
   }
}

void Player::onButtonReleased(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->getInputMap() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->getActionName() + "_RELEASED").c_str(), e.getSender());
   }
}

void Player::onButtonDown(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->getInputMap() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->getActionName() + "_DOWN").c_str(), e.getSender());
   }
}

void Player::onButtonUp(const Event& e)
{
   InputEvent* inputEvent = (InputEvent*)&e;

   if (inputEvent->getInputMap() == this->_pad) {
      this->_object->sendInput(((std::string)((InputEvent*)&e)->getActionName() + "_UP").c_str(), e.getSender());
   }
}

void Player::start(void)
{
	InputMap::EventListener::start(); // Start the controller event listener
}

void Player::update(float time)
{

}

void Player::finish(void)
{
	InputMap::EventListener::finish(); // Finish the controller event listener
}