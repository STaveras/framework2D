#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

class FollowObject: public ObjectOperator
{
      GameObject* _source; // the object following
      GameObject* _object; // the object followed

      vector2 _offset; // TODO
      bool _horizontally, _vertically;

   public:

      void setSource(GameObject *source) { _source = source; }

      void follow(GameObject *object, bool horizontally, bool vertically) { _object = object; _vertically = vertically; _horizontally = horizontally; }

      bool operator()(GameObject *object) {

         if (!_source || (_object && _object != object))
            return true;

         vector2 position = _source->getPosition();

         if (_horizontally)
            position.x = object->getPosition().x;

         if (_vertically)
            position.y = object->getPosition().y;

         _source->setPosition(position);

         return true;
      }
};