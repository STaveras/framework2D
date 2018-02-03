#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

typedef GameObject::GameObjectState ObjectState;

class UpdateRenderableOperator: public ObjectOperator
{
   bool operator()(GameObject* object) {

      ((ObjectState*)object->GetCurrentState())->getRenderable()->SetPosition(object->GetPosition()); return true;
   }
};