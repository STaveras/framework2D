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

   friend class ObjectOperator;

   //void _OnKeyPressed(const Event & e);
   //void _OnKeyReleased(const Event & e);
   //void _OnAnimationStopped(const Event& e);

public:
   class GameObjectState : public State
   {
      friend GameObject;
      Renderable* _renderable;
      Collidable* _Collidable;
      vector2 _Direction;
      double _Force;
      double _executeTime;
      double _runTime;

   public:
      GameObjectState(void) : State(),
         _renderable(NULL),
         _Collidable(NULL),
         _Direction(vector2(0, 0)),
         _Force(0.0),
         _executeTime(0.0),
         _runTime(0.0) {
      }

      Renderable * getRenderable(void) { return _renderable; }
      void setRenderable(Renderable* renderable) { _renderable = renderable; }

      Collidable* GetCollidable(void) { return _Collidable; }
      void SetCollidable(Collidable* collidable) { _Collidable = collidable; }

      vector2 getDirection(void) const { return _Direction; }
      void setDirection(vector2 direction) { /*D3DXNormalize*/ _Direction = direction; }

      double getForce(void) const { return _Force; }
      void setForce(double force) { _Force = force; }

      double getExecuteTime(void) const { return _executeTime; }
      void setExecuteTime(double runTime) { _executeTime = runTime; }

      virtual void onEnter(State* prev);
      virtual bool onExecute(float time);
      virtual void onExit(State* next);
   };

protected:
   float m_fRotation;
   vector2 _velocity;

public:
   GameObject(void) :m_eType(GAME_OBJ_NULL), m_fRotation(0.0f) {}
   GameObject(GAME_OBJ_TYPE eType) :m_eType(eType), m_fRotation(0.0f) {}
   ~GameObject(void) {}

   GAME_OBJ_TYPE GetType(void) const { return m_eType; }
   float GetRotation(void) const { return m_fRotation; }
   vector2 GetVelocity(void) const { return _velocity; }
   void SetRotation(float fRotation) { m_fRotation = fRotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)
   void SetVelocity(vector2 velocity) { _velocity = velocity; }

   GameObjectState* addState(const char* szName);
   //void setAnimation(Animation* ani);
   //void setStateAnimation(const char* stateName, Animation* ani);

   virtual void Setup(void) {}
   virtual void update(float fTime);
   virtual void Shutdown(void) {}

   void AddImpulse(vector2 direction, double force) {
      _velocity += (direction * (float)force);
   }
};
typedef GameObject::GameObjectState ObjectState;
#endif
// Author: Stanley Taveras
