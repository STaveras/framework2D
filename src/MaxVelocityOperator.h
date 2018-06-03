#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

class MaxVelocityOperator: public ObjectOperator
{
   float _maxSpeed;

public:
   float getMaxSpeed(void) const { return _maxSpeed; }
   void setMaxSpeed(float maxSpeed) { _maxSpeed = maxSpeed; }

   bool operator()(GameObject* object) {
      
      // TODO: Write your own functions to wrap the math; there should be NO calls to the d3d math lib.

      vector2 objectVelocity = object->GetVelocity();

      if (D3DXVec2Length(&objectVelocity) >= _maxSpeed) {

         vector2 normalizedVel;
         D3DXVec2Normalize(&normalizedVel, &objectVelocity);

         object->SetVelocity(normalizedVel * _maxSpeed);
      }

      return true;
   }
};