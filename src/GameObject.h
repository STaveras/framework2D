// File: GameObject.h
#pragma once
#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_

#include "StateMachine.h"
#include "Physical.h"

#include "GAME_OBJ_TYPE.h"
#include "Engine2D.h"
#include "AnimationManager.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Types.h"
#include "ObjectOperator.h"

#include <functional>

#define EVT_GAMEOBJECT_STATE_ENTER "EVT_STATE_ENTER"
#define EVT_GAMEOBJECT_STATE_EXIT  "EVT_STATE_EXIT"

class GameObject : public StateMachine, public Physical
{
public:
    enum GAME_OBJ_TYPE
    {
        GAME_OBJ_NULL,
        GAME_OBJ_CAMERA,
        GAME_OBJ_OBJECT,
        GAME_OBJ_TILE,
        GAME_OBJ_EFFECT
    };

private:
   GAME_OBJ_TYPE m_eType = GAME_OBJ_NULL; // I feel like this shouldn't be a thing

   // GameObject is a lil' dim
   friend class ObjectOperator;

protected:
   Factory<Sprite>     _spriteManager;
   Factory<Animation>  _animationManager;

public:
   class GameObjectState : public State
   {
      friend GameObject;
      bool _preserveMirror;
      Renderable* _renderable;
      Collidable* _collidable;
      vector2 _direction;
      double _force;
      double _executeTime;
      double _runTime;

   public:
      GameObjectState(void) : State(),
         _preserveMirror(false),
         _renderable(NULL),
         _collidable(NULL),
         _direction(vector2(0, 0)),
         _force(0.0),
         _executeTime(0.0),
         _runTime(0.0) {
      }

      bool isPreservingMirroring(void) const { return _preserveMirror; }
      void setPreserveMirroring(bool preserveMirror) { _preserveMirror = preserveMirror; }

      Renderable* getRenderable(void) { return _renderable; }
      void setRenderable(Renderable* renderable) { _renderable = renderable; }

      Collidable* getCollidable(void) { return _collidable; }
      void setCollidable(Collidable* collidable) { _collidable = collidable; }

      vector2 getDirection(void) const { return _direction; }
      void setDirection(vector2 direction) { _direction = direction; }

      double getForce(void) const { return _force; }
      void setForce(double force) { _force = force; }

      // TODO: Make execution time work with physics to scale force -- this 'execution time' should be seen as a MAX

      double getExecuteTime(void) const { return _executeTime; }
      void setExecuteTime(double runTime) { _executeTime = runTime; }

      virtual void onEnter(State* prev);
      virtual bool onExecute(float time);
      virtual void onExit(State* next);
   };

protected:

   std::function<void(const CollisionEvent* e)> collisionEventHandler = NULL;

public:
   GameObject(void){ Engine2D::getEventSystem()->registerCallback<GameObject>(EVENT_COLLISION, this, &GameObject::_OnCollision); }
   GameObject(GAME_OBJ_TYPE eType): m_eType(eType) {}
   virtual ~GameObject(void) {}

   GAME_OBJ_TYPE GetType(void) const { return m_eType; }

   GameObjectState* addState(const char* name);
   GameObjectState* getState(const char* name) { return (GameObjectState*)StateMachine::getState(name); }
   GameObjectState* getState(void) const { return (GameObjectState*)StateMachine::getState(); } // Just cus I'm tired of adding (ObjectState*) and whatnot

   Renderable* getRenderable(void) const { return this->getState()->getRenderable(); }
   Collidable* getCollidable(void) const { return this->getState()->getCollidable(); }

   virtual void update(float fTime);
   virtual void shutdown(void) {}

   // TODO: GameObjects should maybe have an overload for operator()
   //       they could take in other objects, and perform collision checks between it and the other object?
   //       maybe do other things... 

private:
    virtual void _OnCollision(const Event& e);
    virtual void _OnKeyPressed(const Event& e);
    virtual void _OnKeyReleased(const Event& e);
};
typedef GameObject::GameObjectState ObjectState;
#endif
// Author: Stanley Taveras
