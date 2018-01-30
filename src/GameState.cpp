// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"

Engine2D* engine = Engine2D::GetInstance();

Sprite * GameState::AddSprite(const char * filename)
{
   Sprite *sprite = _sprites.Create(Sprite(filename));
   _renderList->push_back(sprite);
   return sprite;
}

void GameState::RemoveSprite(Sprite * sprite)
{
   _renderList->remove(sprite);
   _sprites.Destroy(sprite);
}

//void GameState::AddObject(GameObject * object)
//{
//
//}
//
//void GameState::RemoveObject(GameObject * object)
//{
//}

void GameState::OnEnter(void)
{
   _renderList = engine->GetRenderer()->CreateRenderList();

   engine->GetEventSystem()->RegisterCallback<GameState>("EVT_STATE_ENTER", this, &GameState::_OnObjectStateEnter);
   engine->GetEventSystem()->RegisterCallback<GameState>("EVT_STATE_EXIT", this, &GameState::_OnObjectStateExit);

   _AnimationManager.Initialize(_renderList);
   _InputManager.Initialize(engine->GetEventSystem(),
                            engine->GetInput());
}

void GameState::OnExecute(float fTime)
{
   _AnimationManager.Update(fTime);
   _InputManager.Update(fTime);
   _ObjectManager.Update(fTime); // TODO: Build an object cache 
}

void GameState::OnExit(void)
{
   _sprites.Clear();
   _InputManager.Shutdown();
   _AnimationManager.Shutdown();

   engine->GetEventSystem()->Unregister<GameState>("EVT_STATE_EXIT", this, &GameState::_OnObjectStateExit);
   engine->GetEventSystem()->Unregister<GameState>("EVT_STATE_ENTER", this, &GameState::_OnObjectStateEnter);

   engine->GetRenderer()->DestroyRenderList(_renderList);
}

///
// Right now the only object manager that is being updated is this one since it's on the top of the stack...
// However, we'll need to change the event system to send details about the object and the state change.
// Meaning we'll need to filter for the object, checking if it's contained within our object manager
///

void GameState::_OnObjectStateEnter(const Event& e) {
   
   if (e.GetSender()) {

      GameObject::GameObjectState *objectState = (GameObject::GameObjectState*)e.GetSender();

      _renderList->push_back(objectState->GetRenderable());

   }
}

void GameState::_OnObjectStateExit(const Event& e) {

   if (e.GetSender()) {

      GameObject::GameObjectState *objectState = (GameObject::GameObjectState*)e.GetSender();

      _renderList->remove(objectState->GetRenderable());

   }
}

// Author: Stanley Taveras