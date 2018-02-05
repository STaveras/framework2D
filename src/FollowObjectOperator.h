#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

class FollowObject: public ObjectOperator
{
      GameObject* _source; // the object following
      GameObject* _object; // the object followed

      vector2 _offset; // TODO
      bool _vertically, _horizontally;

   public:

      void setSource(GameObject *source) { _source = source; }

      void follow(GameObject *object, bool vertically, bool horizontally) { _object = object; _vertically = vertically; _horizontally = horizontally; }

      bool operator()(GameObject *object) {

         if (!_source || (_object && _object != object))
            return true;

         vector2 position = _source->getPosition();

         if (_vertically)
            position.y = object->getPosition().y;

         if (_horizontally)
            position.x = object->getPosition().x;

         _source->SetPosition(position);

         return true;
      }
};