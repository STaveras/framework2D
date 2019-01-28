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
      _timer.ResetElapsed();
   }

protected:
   bool operator()(GameObject *object) {

      if (_object == object) {

         if (_state == _object->GetCurrentState()) {

            _timer.Update();

            float scalar = 1.0f - (_timer.GetDeltaTime() / _degradeTime);

            _state->setForce(_force * scalar);

            if (_timer.GetElapsedTime() >= _degradeTime)
            {
               // Original force; should we just set to 0, instead...?
               _object->getState()->setForce(_force);

               return false;
            }
         }
         else
            _timer.ResetElapsed();
      }
      return true;
   }
};