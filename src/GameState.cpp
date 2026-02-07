// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"

#include "GameObject.h"

#include <algorithm>

Engine2D* engine = Engine2D::getInstance();

bool GameState::addObject(GameObject * object)
{

   return false; // Not added
}

bool GameState::removeObject(GameObject * object)
{

   return false; // Not removed
}

void GameState::onEnter(State* prevState)
{
   _renderList = engine->getRenderer()->createRenderList();
   _collisionSystem.reset();

   engine->getEventSystem()->registerCallback<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->registerCallback<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);

   _inputManager.initialize(engine->getEventSystem(),
                            engine->getInput());
}

bool GameState::onExecute(float time)
{
   _inputManager.update(time);
   _objectManager.update(time);
   _collisionSystem.update(_objectManager.getObjects());

   return true; // We're still updating!!! ...Right?
}

void GameState::onExit(State* nextState)
{
   _collisionSystem.reset();

   _inputManager.shutdown();

   engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);
   engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);

   engine->getRenderer()->destroyRenderList(_renderList);
}

///
// Right now the only object manager that is being updated is this one since it's on the top of the stack...
// However, we'll need to change the event system to send details about the object and the state change.
// Meaning we'll need to filter for the object, checking if it's contained within our object manager
///

void GameState::_OnObjectAdded(const Event & e)
{
   GameObject* object = (GameObject*)e.getSender();
   if (!object) {
      return;
   }

   object->start();

   GameObject::GameObjectState* currentState = object->getState();
   for (auto it = object->begin(); it != object->end(); ++it) {
      GameObject::GameObjectState* state = (GameObject::GameObjectState*)(*it);
      if (!state) {
         continue;
      }
      Renderable* renderable = state->getRenderable();
      if (renderable && std::find(_renderList->begin(), _renderList->end(), renderable) == _renderList->end()) {
         _renderList->push_back(renderable);
      }
      if (renderable) {
         renderable->setVisibility(state == currentState);
      }
   }
}

void GameState::_OnObjectRemoved(const Event & e)
{
   GameObject* object = (GameObject*)e.getSender();
   if (!object) {
      return;
   }

   for (auto it = object->begin(); it != object->end(); ++it) {
      GameObject::GameObjectState* state = (GameObject::GameObjectState*)(*it);
      if (!state) {
         continue;
      }
      Renderable* renderable = state->getRenderable();
      if (renderable) {
         _renderList->remove(renderable);
      }
   }

   object->finish();
}
