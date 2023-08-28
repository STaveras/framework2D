// File: GameObject.h
#pragma once
#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_

#include "StateMachine.h"
#include "Physical.h"

#include "Engine2D.h"
#include "AnimationManager.h"
#include "Collidable.h"
#include "Renderable.h"
#include "Types.h"
#include "ObjectOperator.h"
#include "Game.h"

#include <functional>

// TODO: GameObjects should maybe have an overload for operator() ?
//       they could take in other objects, and perform collision checks between it and the other object?
//       maybe do other things... 

// TODO: Make execution time work with physics to scale force -- this 'execution time' should be seen as a MAX

// NOTES: So originally I wanted to avoid clich�s and not make this your typical game object class... But sometimes, tropes are a good thing
//        What we have here is something in between a base "object" class and a pseudo "entity" class... Which is why something like the Camera derives from 
//        So this will change over time a lot until I land on something that is sufficiently generalized enough to cover as many different games 

#define EVT_GAMEOBJECT_STATE_ENTER "EVT_STATE_ENTER"
#define EVT_GAMEOBJECT_STATE_EXIT  "EVT_STATE_EXIT"

class GameObject : public StateMachine, public Physical
{
   //Timer _lifeTime; this one makes me sad

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
   GAME_OBJ_TYPE _objType = GAME_OBJ_NULL; // I feel like this shouldn't be a thing

   // GameObject is a lil' dim
   friend class ObjectOperator;

protected:
   Factory<Sprite>     _spriteManager;
   Factory<Animation>  _animationManager;
   Factory<Collidable> _collisionObjects;

public:
   class GameObjectState : public State
   {
      friend GameObject;
      bool _preserveScaling;
      Renderable* _renderable;
      Collidable* _collidable;
      vector2 _direction;
      double _force;
      double _executeTime;
      double _runTime;

   public:
      GameObjectState(void) : State(),
         _preserveScaling(false),
         _renderable(NULL),
         _collidable(NULL),
         _direction(vector2(0, 0)),
         _force(0.0),
         _executeTime(0.0),
         _runTime(0.0) {
      }

      bool isPreservingScale(void) const { return _preserveScaling; }
      void setPreserveScaling(bool preserveMirror) { _preserveScaling = preserveMirror; }

      Renderable* getRenderable(void) { return _renderable; }
      void setRenderable(Renderable* renderable) { _renderable = renderable; }

      Collidable* getCollidable(void);
      void setCollidable(Collidable* collidable) { _collidable = collidable; }

      vector2 getDirection(void) const { return _direction; }
      void setDirection(vector2 direction) { _direction = direction; }

      double getForce(void) const { return _force; }
      void setForce(double force) { _force = force; }

      double getExecuteTime(void) const { return _executeTime; }
      void setExecuteTime(double runTime) { _executeTime = runTime; }

      virtual void onEnter(State* prev);
      virtual bool onExecute(float time);
      virtual void onExit(State* next);
   };

   void setPosition(vector2 position);
   void setPosition(float x, float y) { this->setPosition(vector2(x, y)); }

protected:

   void updateComponents();

   std::function<void(const Event* e)> _collisionEventHandler = NULL;

public:
   // We should probably forgo "GameObject types"
   explicit GameObject(GAME_OBJ_TYPE eType): _objType(eType) {}
   virtual ~GameObject(void) {
      _collisionObjects.clear();
      _animationManager.clear();
      _spriteManager.clear();
   }

   GAME_OBJ_TYPE getType(void) const { return _objType; }

   //void setIsColliding(bool colliding) { _isColliding = colliding; }
   //bool isColliding(void) const { _isColliding; }

   GameObjectState* addState(const char* name);
   GameObjectState* getState(const char* name) { return (GameObjectState*)StateMachine::getState(name); }
   GameObjectState* getState(void) const { return (GameObjectState*)StateMachine::getState(); } // Just cus I'm tired of adding (ObjectState*) and whatnot

   Renderable* getRenderable(void) const { return this->getState()->getRenderable(); }
   Collidable* getCollidable(void);

   virtual void start(void);
   virtual void update(float fTime);
   virtual void finish(void);

private:
    virtual void _onStateEntered(const Event& e);
    virtual void _onStateExited(const Event& e);

    virtual void _onCollision(const Event& e);
};
typedef GameObject::GameObjectState ObjectState;
#endif
