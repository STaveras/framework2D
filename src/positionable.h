#pragma once

#include "Types.h"

class Positionable {

protected:
   vector2 _position;

public:
   Positionable(void) :
      _position(vector2(0, 0)) {}
   Positionable(float x, float y) :
      _position(x, y) {}
   Positionable(vector2 position) :
      _position(position) {}

   virtual ~Positionable(void) = 0{ }

   vector2 getPosition(void) const { return _position; }
   void setPosition(float x, float y) { _position.x = x; _position.y = y; }
   void setPosition(vector2 position) { _position = position; }
};
