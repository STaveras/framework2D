#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"
#include "Timer.h"

// All good things come to an end... (Or does they/it?)
class ForceDegradationOperator : public ObjectOperator
{
   float _force;
   float _degradeTime;
   GameObject *_object;
   GameObject::GameObjectState* _state;
   Timer _timer;

public:
   ForceDegradationOperator(float time, GameObject *targetObject):_degradeTime(time)
   {
      _object = targetObject;
      _state = targetObject->getState();
      _force = _state->getForce();
      _timer.reset();
   }

protected:
   bool operator()(GameObject *object) {

      if (_object == object) {

         if (_state == _object->getState()) {

            _timer.update();

            float scalar = 1.0f - (_timer.getDeltaTime() / _degradeTime);

            _state->setForce(_force * scalar);

            if (_timer.getElapsedTime() >= _degradeTime)
            {
               // Original force; should we just set to 0, instead...?
               _object->getState()->setForce(_force);

               return false;
            }
         }
         else
            _timer.reset();
      }
      return true;
   }
};