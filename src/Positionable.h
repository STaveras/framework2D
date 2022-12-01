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

   virtual ~Positionable(void) = default; // ARGH!!!!

   virtual vector2 getPosition(void) const { return _position; }
   virtual void setPosition(float x, float y) { _position.x = x; _position.y = y; }
   virtual void setPosition(vector2 position) { _position = position; }
};
