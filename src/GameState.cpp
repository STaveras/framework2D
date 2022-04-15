// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"

Engine2D* engine = Engine2D::getInstance();

Sprite* GameState::addSprite(const char * filename, color clearColor, RECT* srcRect)
{
   Sprite* sprite = _sprites.Create(Sprite(filename, clearColor, srcRect));
   _renderList->push_back(sprite);
   return sprite;
}

void GameState::removeSprite(Sprite * sprite)
{
   _renderList->remove(sprite);
   _sprites.Destroy(sprite);
}

//void GameState::addObject(GameObject * object)
//{
//
//}
//
//void GameState::removeObject(GameObject * object)
//{
//}

void GameState::onEnter(void)
{
   _renderList = engine->GetRenderer()->CreateRenderList();

   engine->getEventSystem()->RegisterCallback<GameState>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameState::_OnObjectStateEnter);
   engine->getEventSystem()->RegisterCallback<GameState>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameState::_OnObjectStateExit);
   engine->getEventSystem()->RegisterCallback<GameState>("EVT_OBJECT_ADDED", this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->RegisterCallback<GameState>("EVT_OBJECT_REMOVED", this, &GameState::_OnObjectAdded);

   _animationManager.Initialize(_renderList);
   _inputManager.Initialize(engine->getEventSystem(),
                            engine->GetInput());
}

void GameState::onExecute(float fTime)
{
   _animationManager.Update(fTime);
   _inputManager.Update(fTime);
   _objectManager.Update(fTime); // TODO: Build an object cache 
}

void GameState::onExit(void)
{
   _inputManager.Shutdown();
   _animationManager.Shutdown();

   engine->getEventSystem()->Unregister<GameState>("EVT_OBJECT_REMOVED", this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->Unregister<GameState>("EVT_OBJECT_ADDED", this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->Unregister<GameState>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameState::_OnObjectStateExit);
   engine->getEventSystem()->Unregister<GameState>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameState::_OnObjectStateEnter);

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

      _renderList->push_back(objectState->getRenderable());

   }
}

void GameState::_OnObjectStateExit(const Event& e) {

   if (e.GetSender()) {

      GameObject::GameObjectState *objectState = (GameObject::GameObjectState*)e.GetSender();

      _renderList->remove(objectState->getRenderable());

   }
}

void GameState::_OnObjectAdded(const Event & e)
{
   // Do something
   // TODO: GameState::_OnObjectAdded (y tho)
   //throw "GameState::_OnObjectAdded unimplemented";
}

void GameState::_OnObjectRemoved(const Event & e)
{
   // Do something
   // TODO: GameState::_OnObjectRemoved (???)
   //throw "GameState::_OnObjectRemoved unimplemented";
}

// Author: Stanley Taveras