#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

class MaxVelocityOperator : public ObjectOperator
{
   float _maxSpeed;

public:
   float getMaxSpeed(void) const { return _maxSpeed; }
   void setMaxSpeed(float maxSpeed) { _maxSpeed = maxSpeed; }

   bool operator()(GameObject *object)
   {
      vector2 objectVelocity = object->getVelocity();

#if defined(GLM) || defined(__APPLE__)
      if (glm::length(objectVelocity) >= _maxSpeed) {
         objectVelocity = glm::normalize(objectVelocity);
         object->SetVelocity(objectVelocity * _maxSpeed);
      }
#else
      if (D3DXVec2Length(&objectVelocity) >= _maxSpeed)
      {

         vector2 normalizedVel;
         D3DXVec2Normalize(&normalizedVel, &objectVelocity);

         object->setVelocity(normalizedVel * _maxSpeed);
      }
#endif
      return true;
   }
};