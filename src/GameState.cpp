// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"

Engine2D* engine = Engine2D::getInstance();

bool GameState::addObject(GameObject * object)
{
   return false; // Not added
}

bool GameState::removeObject(GameObject * object)
{
   return false; // Not removed
}

void GameState::onEnter(void)
{
   _renderList = engine->getRenderer()->createRenderList();

   engine->getEventSystem()->registerCallback<GameState>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameState::_OnObjectStateEnter);
   engine->getEventSystem()->registerCallback<GameState>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameState::_OnObjectStateExit);
   engine->getEventSystem()->registerCallback<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->registerCallback<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);

   _inputManager.initialize(engine->getEventSystem(),
                            engine->getInput());
}

bool GameState::onExecute(float time)
{
   _inputManager.update(time);
   _objectManager.update(time);

   return true; // We're still updating!!! ...Right?
}

void GameState::onExit(void)
{
   _inputManager.shutdown();

   engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);
   engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->unregister<GameState>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameState::_OnObjectStateExit);
   engine->getEventSystem()->unregister<GameState>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameState::_OnObjectStateEnter);

   engine->getRenderer()->destroyRenderList(_renderList);
}

///
// Right now the only object manager that is being updated is this one since it's on the top of the stack...
// However, we'll need to change the event system to send details about the object and the state change.
// Meaning we'll need to filter for the object, checking if it's contained within our object manager
///

void GameState::_OnObjectStateEnter(const Event& e) {

   if (e.getSender()) {

      GameObject::GameObjectState *objectState = (GameObject::GameObjectState*)e.getSender(); 
      Renderable* renderable = objectState->getRenderable();

      if (renderable) {
         _renderList->push_back(renderable);
      }
   }
}

void GameState::_OnObjectStateExit(const Event& e) {

   if (e.getSender()) {

      GameObject::GameObjectState *objectState = (GameObject::GameObjectState*)e.getSender();
      Renderable* renderable = objectState->getRenderable();

      if (renderable) {
          _renderList->remove(renderable);
      }
   }
}

void GameState::_OnObjectAdded(const Event & e)
{
   ((GameObject*)e.getSender())->start();
}

void GameState::_OnObjectRemoved(const Event & e)
{
   ((GameObject*)e.getSender())->finish();
}
