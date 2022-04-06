#include "GameObject.h"
#include "Animation.h"
#include "InputEvent.h"

GameObject::GameObjectState* GameObject::addState(const char* szName)
{
   GameObjectState* state = _states.CreateDerived<GameObjectState>(); // TODO: Some way for the factory to take constructor arguments
   state->SetName(szName);

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
      }
   }

   Engine2D::getInstance()->getEventSystem()->sendEvent("EVT_STATE_ENTER", this);
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
   Engine2D::getInstance()->getEventSystem()->sendEvent("EVT_STATE_EXIT", this);

   if (_renderable) {
      switch (_renderable->getRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
         ((Animation*)_renderable)->Stop();
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