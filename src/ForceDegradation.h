#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"
#include "Timer.h"

// All good things come to an end... (Or does it?)
class ForceDegradationOperator : public ObjectOperator
{
   float _degradeTime;
   GameObject *_object;
   State* _state; // the original state
   Timer _timer;

public:
   ForceDegradationOperator(float time, GameObject *targetObject):_degradeTime(time)
   {
      _object = targetObject;
      _timer.ResetElapsed();
   }

protected:
   bool operator()(GameObject *object) {

      if (_object == object) {

         _timer.Update();
         
         //_timer.GetDeltaTime

         if (_timer.GetElapsedTime() >= _degradeTime)
            return false;
      }
      return true;
   }
};