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
//	for (unsigned int i = 0; i < m_States.Size(); i++)
//		if (m_States.At(i) == (GameObjectState*)e.GetSender())
//			return this->SendInput("ANIMATION_STOPPED");
//}

GameObject::GameObjectState* GameObject::AddState(const char* szName)
{
   GameObjectState* state = m_States.CreateDerived<GameObjectState>(); // TODO: Some way for the factory to take constructor arguments
   state->SetName(szName);

   return state;
}

void GameObject::SetAnimation(Animation* ani)
{
   // TODO: Should set the CURRENT state's animation
   if (m_States.Size())
      ((GameObjectState*)m_States[m_States.Size() - 1])->setRenderable(ani);
   else
      throw "GameObject: Does not have any states.";
}

void GameObject::SetStateAnimation(const char* stateName, Animation* ani)
{
   GameObjectState* state = (GameObjectState*)GetState(stateName);
   state->_Renderable = ani;
}

void GameObject::GameObjectState::OnEnter(State* prev)
{
   _RunTime = 0;

   if (_Renderable) {
      _Renderable->SetVisibility(true);

      switch (_Renderable->GetRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
         ((Animation*)_Renderable)->Play();
         break;
      }
   }

   Engine2D::GetInstance()->GetEventSystem()->SendEvent("EVT_STATE_ENTER", this);
}

bool GameObject::GameObjectState::OnExecute(float time)
{
   _RunTime += time;

   if (_Renderable) {
      switch (_Renderable->GetRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
      {
         Animation* animation = (Animation*)_Renderable;

         if (animation->Update(time))
            return true;
         else
            return false;
      }
      break;
      }
   }

   if (_ExecuteTime > 0 && _RunTime >= _ExecuteTime) {
      return false;
   }

   return true;
}

void GameObject::GameObjectState::OnExit(State* next)
{
   Engine2D::GetInstance()->GetEventSystem()->SendEvent("EVT_STATE_EXIT", this);

   if (_Renderable) {
      switch (_Renderable->GetRenderableType())
      {
      case RENDERABLE_TYPE_ANIMATION:
         ((Animation*)_Renderable)->Stop();
         break;
      }

      _Renderable->SetVisibility(false);
   }
}

//void GameObject::Setup(void)
//{
//   // Collision events
//   //Engine2D::GetInstance()->GetEventSystem()->RegisterCallback<GameObject>("EVT_KEYPRESSED", this, &GameObject::_OnKeyPressed);
//   //Engine2D::GetInstance()->GetEventSystem()->RegisterCallback<GameObject>("EVT_KEYRELEASED", this, &GameObject::_OnKeyReleased);
//}
//
//void GameObject::Shutdown(void)
//{
//   //Engine2D::GetInstance()->GetEventSystem()->UnregisterAll<GameObject>(this, &GameObject::_OnKeyReleased);
//   //Engine2D::GetInstance()->GetEventSystem()->UnregisterAll<GameObject>(this, &GameObject::_OnKeyPressed);
//}

//void GameObject::Initialize()
//{
//   StateMachine::Initialize();
//}

void GameObject::Update(float fTime)
{
   StateMachine::Update(fTime);
   
   m_Position += m_Velocity * fTime; 

   GameObjectState* currentState = (GameObjectState*)this->GetCurrentState();
   
   currentState->GetRenderable()->SetPosition(m_Position);
   //currentState->GetRenderable()->Set
}