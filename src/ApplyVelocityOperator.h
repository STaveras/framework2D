#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

// DEPRECATED: Legacy operator kept for compatibility while movement stays in Character/Physics.
class ApplyVelocityOperator: public ObjectOperator
{
public:
   bool operator()(GameObject* object)
   {
      ObjectState* objectState = (ObjectState*)object->getState();

      if (objectState) {
         object->SetVelocity(objectState->getDirection() * (float)objectState->getForce());
      }
      return true;
   }
};
