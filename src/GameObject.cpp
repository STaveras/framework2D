#include "GameObject.h"
#include "Animation.h"
#include "InputEvent.h"

//void GameObject::_OnKeyPressed(const Event& e)
//{
//   this->SendInput(((std::string)((InputEvent*)&e)->GetButtonID() + "_PRESSED").c_str(), e.GetSender());
//}
//
//void GameObject::_OnKeyReleased(const Event& e)
//{
//   this->SendInput(((std::string)((InputEvent*)&e)->GetButtonID() + "_RELEASED").c_str(), e.GetSender());
//}

//void GameObject::_OnAnimationStopped(const Event& e)
//{
//	for (unsigned int i = 0; i < _states.Size(); i++)
//		if (_states.At(i) == (GameObjectState*)e.GetSender())
//			return this->SendInput("ANIMATION_STOPPED");
//}

GameObject::GameObjectState* GameObject::addState(const char* szName)
{
   GameObjectState* state = _states.CreateDerived<GameObjectState>(); // TODO: Some way for the factory to take constructor arguments
   state->SetName(szName);

   return state;
}

//void GameObject::setAnimation(Animation* ani)
//{
//   // TODO: Should set the CURRENT state's animation
//   if (_states.Size())
//      ((GameObjectState*)_states[_states.Size() - 1])->setRenderable(ani);
//   else
//      throw "GameObject: Does not have any states.";
//}
//
//void GameObject::setStateAnimation(const char* stateName, Animation* ani)
//{
//   ((GameObjectState*)GetState(stateName))->_renderable = ani;
//}

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

   if (_renderable) {
      switch (_renderable->getRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
      {
         Animation* animation = (Animation*)_renderable;

         if (animation->update(time))
            return true;
         else
            return false;
      }
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

//void GameObject::setup(void)
//{
//   // Collision events
//   //Engine2D::getInstance()->getEventSystem()->RegisterCallback<GameObject>("EVT_KEYPRESSED", this, &GameObject::_OnKeyPressed);
//   //Engine2D::getInstance()->getEventSystem()->RegisterCallback<GameObject>("EVT_KEYRELEASED", this, &GameObject::_OnKeyReleased);
//}
//
//void GameObject::shutdown(void)
//{
//   //Engine2D::getInstance()->getEventSystem()->UnregisterAll<GameObject>(this, &GameObject::_OnKeyReleased);
//   //Engine2D::getInstance()->getEventSystem()->UnregisterAll<GameObject>(this, &GameObject::_OnKeyPressed);
//}

//void GameObject::Initialize()
//{
//   StateMachine::Initialize();
//}

void GameObject::update(float time) // lawl time as a float xfd
{
   StateMachine::update(time); _position += _velocity * time;
}