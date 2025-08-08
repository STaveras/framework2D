#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"

// This is more like "hard attach"
class AttachObjectsOperator: public ObjectOperator
{
	// The description seems backwards, but is as follows:
      GameObject* _source = NULL; // the object following
      GameObject* _object = NULL; // the object followed

      vector2 _offset;
	  bool _horizontally = true, _vertically = true; // we should just send a scalar vector2 instead of two bools

   public:

      void setSource(GameObject *source) { _source = source; }

      void follow(GameObject *object, bool horizontally, bool vertically) { _object = object; _vertically = vertically; _horizontally = horizontally; }

      bool operator()(GameObject *object) {

         if (!_source || (_object && _object != object))
            return true;

         vector2 position = _source->getPosition();

         if (_horizontally) // we would compare with (1, 0)
            position.x = object->getPosition().x;

		 if (_vertically) // we would compare with (0, 1)
            position.y = object->getPosition().y;

         _source->setPosition(position + _offset);

         return true;
      }
};