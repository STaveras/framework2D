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
   _renderList = engine->getRenderer()->CreateRenderList();

   engine->getEventSystem()->registerCallback<GameState>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameState::_OnObjectStateEnter);
   engine->getEventSystem()->registerCallback<GameState>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameState::_OnObjectStateExit);
   engine->getEventSystem()->registerCallback<GameState>(EVENT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->registerCallback<GameState>(EVENT_OBJECT_REMOVED, this, &GameState::_OnObjectAdded);

   _inputManager.initialize(engine->getEventSystem(),
                            engine->getInput());
}

bool GameState::onExecute(float time)
{
#if _DEBUG
   timer += time;

   if (timer >= 1.0f) {

      if (Debug::dbgObjects) {

         char debugBuffer[255];

         // OutputDebugString("\nObjects:\n-------\n");

         // Debug::Log << "\nObjects:\n-------\n";
         Debug::Log->write("\nObjects:\n-------\n");

         for (unsigned int i = 0; i < _objectManager.numObjects(); i++) {
            GameObject* debugObject = _objectManager[i];
            sprintf_s(debugBuffer, "(%s) pos: (%fx, %fy)\n", _objectManager.getObjectName(debugObject).c_str(), debugObject->getPosition().x, debugObject->getPosition().y);
            // OutputDebugString(debugBuffer);
            Debug::Log->write(debugBuffer);
         }
      }

      if (_camera) {
         if (!_camera->OnScreen(_player->getGameObject())) {
            //Engine2D::getEventSystem()->sendEvent("EVT_GAME_OVER");
            // OutputDebugString("GameObject off screen\n\n");
            Debug::Log->write("GameObject off screen\n\n");
         }
      }

      timer = 0.0f;
   }
#endif

   _inputManager.update(time);
   _objectManager.update(time); // TODO: Build an object graph 

   return true; // We're still updating!!! ...Right?
}

void GameState::onExit(void)
{
   _inputManager.shutdown();

   engine->getEventSystem()->unregister<GameState>(EVENT_OBJECT_REMOVED, this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->unregister<GameState>(EVENT_OBJECT_REMOVED, this, &GameState::_OnObjectAdded);
   engine->getEventSystem()->unregister<GameState>(EVT_GAMEOBJECT_STATE_EXIT, this, &GameState::_OnObjectStateExit);
   engine->getEventSystem()->unregister<GameState>(EVT_GAMEOBJECT_STATE_ENTER, this, &GameState::_OnObjectStateEnter);

   engine->getRenderer()->DestroyRenderList(_renderList);
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
   //throw std::runtime_error("GameState::_OnObjectAdded unimplemented");
}

void GameState::_OnObjectRemoved(const Event & e)
{
   // Do something
   //throw std::runtime_error("GameState::_OnObjectRemoved unimplemented");
}

// Author: Stanley Taveras