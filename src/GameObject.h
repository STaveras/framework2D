// File: GameObject.h
#pragma once
#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_
#include "GAME_OBJ_TYPE.h"
#include "Renderable.h"
#include "StateMachine.h"
#include "Types.h"
#include "ObjectOperator.h"

class Collidable;

class GameObject : public StateMachine, public Physical
{
   GAME_OBJ_TYPE m_eType; // I feel like this shouldn't be a thing

   // GameObject is a lil' dim
   friend class ObjectOperator;

protected:
   Factory<ObjectOperator> _specialOpsFactory; // lmao

public:
   class GameObjectState : public State
   {
      friend GameObject;
      Renderable* _renderable;
      Collidable* _collidable;
      vector2 _direction;
      double _force;
      double _executeTime;
      double _runTime;

   public:
      GameObjectState(void) : State(),
         _renderable(NULL),
         _collidable(NULL),
         _direction(vector2(0, 0)),
         _force(0.0),
         _executeTime(0.0),
         _runTime(0.0) {
      }

      Renderable* getRenderable(void) { return _renderable; }
      void setRenderable(Renderable* renderable) { _renderable = renderable; }

      Collidable* getCollidable(void) { return _collidable; }
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

protected:

public:
   GameObject(void) :m_eType(GAME_OBJ_NULL) {}
   GameObject(GAME_OBJ_TYPE eType) :m_eType(eType) {}
   ~GameObject(void) {}

   GAME_OBJ_TYPE GetType(void) const { return m_eType; }

   GameObjectState* addState(const char* szName);
   GameObjectState* getState(void) const { return (GameObjectState*)this->GetCurrentState(); } // Just cus I'm tired of adding (ObjectState*) and whatnot

   Renderable* getRenderable(void) const { return this->getState()->getRenderable(); }
   Collidable* getCollisionInfo(void) const { return this->getState()->getCollidable(); }

   virtual void initialize(void) { StateMachine::initialize(); }
   virtual void update(float fTime);
   virtual void Shutdown(void) {}

   // TODO: GameObjects should maybe have an overload for operator()
   //       they could take in other objects, and perform collision checks between it and the other object?
   //       maybe do other things... 

private:
    virtual void GameObject::_OnKeyPressed(const Event& e);
    virtual void GameObject::_OnKeyReleased(const Event& e);
    virtual void GameObject::_OnAnimationStopped(const Event& e);
};
typedef GameObject::GameObjectState ObjectState;
#endif
// Author: Stanley Taveras
