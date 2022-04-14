#include "GameObject.h"
#include "Animation.h"
#include "InputEvent.h"

void GameObject::_OnKeyPressed(const Event& e)
{
   this->SendInput(((std::string)((InputEvent*)&e)->GetButtonID() + "_PRESSED").c_str(), e.GetSender());
}

void GameObject::_OnKeyReleased(const Event& e)
{
   this->SendInput(((std::string)((InputEvent*)&e)->GetButtonID() + "_RELEASED").c_str(), e.GetSender());
}

void GameObject::_OnAnimationStopped(const Event& e)
{
	for (unsigned int i = 0; i < _states.Size(); i++)
		if (_states.At(i) == (GameObjectState*)e.GetSender())
			return this->SendInput("ANIMATION_STOPPED");
}

GameObject::GameObjectState* GameObject::addState(const char* szName)
{
    // TODO: Some way for the factory to take constructor arguments
   GameObjectState* state = (GameObjectState*)this->GetState(szName);
   if (!state) {
      state = _states.CreateDerived<GameObjectState>(); 
      state->SetName(szName);
   }
   return state;
}

void GameObject::GameObjectState::onEnter(State* prev)
{
   _runTime = 0;

   if (_renderable) {

      _renderable->setVisibility(true);

      switch (_renderable->getRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
         ((Animation*)_renderable)->Play();
         break;
      default:
         break;
      }
   }

   Engine2D::getInstance()->getEventSystem()->sendEvent(EVT_GAMEOBJECT_STATE_ENTER, this);
}

bool GameObject::GameObjectState::onExecute(float time)
{
	 _runTime += time;

	 if (_renderable)
    {
		  switch (_renderable->getRenderableType())
		  {
		  case RENDERABLE_TYPE_ANIMATION:
		  {
				Animation* animation = (Animation*)_renderable;
				return animation->update(time);
		  }
		  default:
				break;
		  }
	 }

	 if (_executeTime > 0 && _runTime >= _executeTime) {
		  return false; // state no longer valid
	 }

	 return true;
}

void GameObject::GameObjectState::onExit(State* next)
{
   Engine2D::getInstance()->getEventSystem()->sendEvent(EVT_GAMEOBJECT_STATE_EXIT, this);

   if (_renderable) {
      switch (_renderable->getRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
         ((Animation*)_renderable)->Stop();
         break;
      default:
         break;
      }

      _renderable->setVisibility(false);
   }
}

void GameObject::update(float time) // lawl time as a float xfd
{
   Physical::update(time);
   StateMachine::update(time);
}