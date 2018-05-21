#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

// TODO: Render collision area in debug build only

class UpdateCollidable: public ObjectOperator
{
public:
   bool operator()(GameObject* object) {

      ObjectState *state = (ObjectState*)object->GetCurrentState();

      if (state && state->getRenderable()) {
         //state->setCollidable(state->)
      }
   }
};