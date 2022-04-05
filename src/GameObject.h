// File: GameObject.h
#pragma once
#ifndef _GAMEOBJECT_H_
#define _GAMEOBJECT_H_
#include "GAME_OBJ_TYPE.h"
#include "Renderable.h"
#include "StateMachine.h"
#include "Types.h"

class Collidable;

class GameObject : public StateMachine, public Positionable
{
   GAME_OBJ_TYPE m_eType;

   // GameObject is a lil' dim
   friend class ObjectOperator;

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
      void setDirection(vector2 direction) { /*D3DXNormalize*/ _direction = direction; }

      double getForce(void) const { return _force; }
      void setForce(double force) { _force = force; }

      double getExecuteTime(void) const { return _executeTime; }
      void setExecuteTime(double runTime) { _executeTime = runTime; }

      virtual void onEnter(State* prev);
      virtual bool onExecute(float time);
      virtual void onExit(State* next);
   };

protected:
   float _mass;
   float _rotation;
   vector2 _velocity;

public:
   GameObject(void) : m_eType(GAME_OBJ_NULL), _rotation(0.0f) {}
   GameObject(GAME_OBJ_TYPE eType) :m_eType(eType), _rotation(0.0f) {}
   ~GameObject(void) {}

   GAME_OBJ_TYPE GetType(void) const { return m_eType; }
   float GetRotation(void) const { return _rotation; }
   vector2 GetVelocity(void) const { return _velocity; }
   void SetRotation(float rotation) { _rotation = rotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)
   void SetVelocity(vector2 velocity) { _velocity = velocity; }

   GameObjectState* addState(const char* szName);
   //void setAnimation(Animation* ani);
   //void setStateAnimation(const char* stateName, Animation* ani);

   GameObjectState* getState(void) const { return (GameObjectState*)this->GetCurrentState(); } // Just cus I'm tired of adding (ObjectState*) and whatnot

   virtual void Setup(void) {}
   virtual void update(float fTime);
   virtual void Shutdown(void) {}

   void AddImpulse(vector2 direction, double force) {
      _velocity += (direction * (float)force);
   }

   // TODO: GameObjects should maybe have an overload for operator()
   //       they could take in other objects, and perform collision checks between it and the other object?
   //       maybe do other things... 

private:
    virtual void _OnKeyPressed(const Event& e);
    virtual void _OnKeyReleased(const Event& e);
    virtual void _OnAnimationStopped(const Event& e);
};
typedef GameObject::GameObjectState ObjectState;
#endif
// Author: Stanley Taveras
