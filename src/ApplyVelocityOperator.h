#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

class ApplyVelocityOperator: public ObjectOperator
{
public:
   bool operator()(GameObject* object)
   {
      ObjectState* objectState = (ObjectState*)object->GetCurrentState();

      if (objectState) {
         object->SetVelocity(objectState->getDirection() * (float)objectState->getForce());
      }
      return true;
   }
};