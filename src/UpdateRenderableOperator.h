#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

// TODO: Should also update the collidable info
// DEPRECATED: Legacy operator kept for compatibility.

class UpdateRenderableOperator: public ObjectOperator
{
   bool operator()(GameObject* object) {

      ObjectState* objectState = (ObjectState*)object->getState();

      if (objectState)
         objectState->getRenderable()->setPosition(object->getPosition());
      
      return true;
   }
};

class UpdateCollidableOperator: public ObjectOperator
{
};
